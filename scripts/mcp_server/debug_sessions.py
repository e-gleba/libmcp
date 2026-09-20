"""Long-lived lldb debug sessions: launch, breakpoints, stepping, inspection.

Unlike the one-shot batch tools in debuggers.py, sessions keep a live
inferior across tool calls: debug_start creates the session, debug_run
launches, debug_step/debug_continue advance it, debug_where/debug_vars
inspect it, debug_stop ends it. All blocking SB calls run in a worker
thread with a timeout; on timeout the inferior is stopped (not killed)
so the session stays alive and the call reports clearly instead of
hanging the MCP call. Requires lldb with its Python API (found via `lldb -P`).
"""

from __future__ import annotations

import shutil
import subprocess
import sys
import tempfile
import threading
import uuid
from collections.abc import Callable
from pathlib import Path

from . import repo as repo_paths
from .config import DEBUG_TIMEOUT_SECONDS, ROOT, STDIN_MAX_CHARS
from .debuggers import validate_args

_LLDB = None
_LLDB_ERROR = ""
_MAX_SESSIONS = 8
_MAX_VARS = 200
_MAX_VALUE_CHARS = 500


def _lldb():
    """Import the lldb API via the path reported by the lldb driver.

    The lldb package initializes the debugger at import time, including
    a SIGINT handler, so the first import must run on the main thread;
    FastMCP executes tools on worker threads. The cached fast path is
    safe from any thread. Call warmup() once at server startup.
    """
    global _LLDB, _LLDB_ERROR
    if _LLDB is not None:
        return _LLDB
    if _LLDB_ERROR:
        raise ValueError(f"lldb Python API unavailable: {_LLDB_ERROR}")
    if threading.current_thread() is not threading.main_thread():
        raise ValueError(
            "lldb Python API unavailable: first import must run on the main "
            "thread (lldb installs signal handlers at import time). Restart "
            "the server so warmup() pre-imports it."
        )
    driver = shutil.which("lldb")
    if driver is None:
        _LLDB_ERROR = "lldb is not installed"
        raise ValueError(f"lldb Python API unavailable: {_LLDB_ERROR}")
    try:
        result = subprocess.run(
            [driver, "-P"],
            check=False,
            capture_output=True,
            text=True,
            stdin=subprocess.DEVNULL,
            timeout=30,
        )
        module_path = result.stdout.strip().splitlines()[-1]
        if result.returncode != 0 or not module_path:
            raise OSError("lldb -P reported no module path")
        if module_path not in sys.path:
            sys.path.insert(0, module_path)
        import lldb  # noqa: PLC0415 - path discovered at runtime

        _LLDB = lldb
    except (OSError, subprocess.TimeoutExpired, ImportError, IndexError) as error:
        _LLDB_ERROR = str(error)
        raise ValueError(f"lldb Python API unavailable: {_LLDB_ERROR}") from error
    return _LLDB


def warmup() -> bool:
    """Import and initialize lldb now; call once on the main thread at startup.

    Returns True when the API is ready. Never raises: lldb is optional,
    and tool calls report its absence with a clear error instead.
    """
    try:
        _lldb()
    except Exception:  # noqa: BLE001 - warmup is best-effort by design
        return False
    return True


class _Session:
    """One live debugger, target, and inferior plus stdio spool bookkeeping."""

    def __init__(
        self,
        session_id: str,
        program: Path,
        args: list[str],
        stdin_file: str | None,
        stdout_file: str,
        stderr_file: str,
    ) -> None:
        lldb = _lldb()
        self.id = session_id
        self.program_label = repo_paths.relative_posix(program)
        self.debugger = lldb.SBDebugger.Create()
        self.debugger.SetAsync(False)
        self.target = self.debugger.CreateTarget(str(program))
        if not self.target.IsValid():
            raise ValueError(f"lldb cannot load target: {self.program_label}")
        launch = lldb.SBLaunchInfo([str(program), *args])
        launch.SetWorkingDirectory(str(ROOT))
        if stdin_file is not None:
            launch.AddOpenFileAction(0, stdin_file, True, False)
        # Always capture the inferior's own output: without this it
        # inherits the server's stdio pipe and corrupts the MCP stream.
        launch.AddOpenFileAction(1, stdout_file, False, True)
        launch.AddOpenFileAction(2, stderr_file, False, True)
        self._launch = launch
        self.process = None
        self.launched = False
        self.breakpoints: dict[int, str] = {}
        self.stdin_file = stdin_file
        self.stdout_file = stdout_file
        self.stderr_file = stderr_file

    def live(self) -> bool:
        """Check whether the inferior is currently running or stopped."""
        if self.process is None or not self.process.IsValid():
            return False
        return self.process.GetState() not in _dead_states()

    def finished(self) -> bool:
        """Return True if the session launched and the inferior is now gone."""
        return self.launched and not self.live()


def _dead_states() -> frozenset:
    """Process states in which run/continue/step cannot proceed."""
    lldb = _lldb()
    return frozenset(
        {
            lldb.eStateExited,
            lldb.eStateCrashed,
            lldb.eStateDetached,
        }
    )


_SESSIONS: dict[str, _Session] = {}
_SESSIONS_LOCK = threading.Lock()


def _sweep() -> None:
    """Destroy debuggers of finished sessions to free slots.

    Sessions that were created but never launched are preserved: they
    hold no inferior and cost one SBDebugger, and sweeping them is what
    forced clients to restart in a brand-new session.
    """
    dead = [key for key, item in _SESSIONS.items() if item.finished()]
    for key in dead:
        item = _SESSIONS.pop(key)
        _destroy(item)


def _destroy(item: _Session) -> None:
    """Kill the inferior, remove the stdio spools, destroy the debugger."""
    try:
        if item.process is not None and item.process.IsValid():
            item.process.Kill()
    except Exception:  # noqa: BLE001 - best-effort teardown
        pass
    for spool in (item.stdin_file, item.stdout_file, item.stderr_file):
        if spool is not None:
            Path(spool).unlink(missing_ok=True)
    _lldb().SBDebugger.Destroy(item.debugger)


def _get(session_id: str) -> _Session:
    """Look up a session or raise a helpful error."""
    with _SESSIONS_LOCK:
        item = _SESSIONS.get(session_id)
    if item is None:
        known = ", ".join(sorted(_SESSIONS)) if _SESSIONS else "(none)"
        raise ValueError(f"Unknown debug session: {session_id}. Active: {known}.")
    return item


def _blocking(item: _Session, action: Callable[[], object], timeout: int):
    """Run a blocking debugger call; interrupt (not kill) on timeout.

    On timeout the inferior is stopped so the session stays alive and the
    client can retry with a larger timeout_seconds instead of starting
    over. Only debug_stop destroys the session.
    """
    outcome: dict = {}

    def run() -> None:
        try:
            outcome["value"] = action()
        except Exception as error:  # noqa: BLE001 - reported, not raised
            outcome["error"] = str(error)

    worker = threading.Thread(target=run, daemon=True)
    worker.start()
    worker.join(timeout)
    if worker.is_alive():
        try:
            process = item.process
            if process is not None and process.IsValid():
                process.Stop()
        except Exception:  # noqa: BLE001 - best-effort interrupt
            pass
        worker.join(10)
        raise TimeoutError(
            f"debugger call exceeded {timeout} seconds; "
            "inferior stopped, session preserved - "
            "retry with a larger timeout_seconds"
        )
    if "error" in outcome:
        raise RuntimeError(outcome["error"])
    return outcome.get("value")


def _location(frame) -> str:
    """Render function, file, line, and PC for a stack frame."""
    name = frame.GetFunctionName() or "??"
    try:
        pc = f" [pc 0x{frame.GetPC():x}]"
    except Exception:  # noqa: BLE001 - exotic frames
        pc = ""
    entry = frame.GetLineEntry()
    if entry.IsValid():
        return f"{name} at {entry.GetFileSpec().GetFilename()}:{entry.GetLine()}{pc}"
    return f"{name}{pc}"


def _render_value(value) -> str:
    """Render one variable as `name = value (type)`."""
    return f"{value.GetName()} = {_value_text(value)} ({value.GetTypeName()})"


def _frame_summary(frame, *, arguments_only: bool, limit: int) -> list[str]:
    """Collect bounded variable lines for one frame; never raises."""
    try:
        values = frame.GetVariables(arguments_only, not arguments_only, False, True)
    except Exception:  # noqa: BLE001 - frame may be gone
        return []
    lines = []
    for value in values:
        if len(lines) >= limit:
            lines.append(f"... truncated at {limit}")
            break
        try:
            lines.append(_render_value(value))
        except Exception:  # noqa: BLE001, S112 - skip unreadable values
            continue
    return lines


def _stop_reason(thread) -> str:
    """Describe why a thread stopped, falling back to the numeric reason."""
    try:
        text = thread.GetStopDescription(256)
    except Exception:  # noqa: BLE001 - older bindings differ
        text = ""
    return text or f"reason={thread.GetStopReason()}"


def _inferior_output(item: _Session, limit: int = 2000) -> str | None:
    """Return the bounded tail of the inferior's captured stdout/stderr."""
    chunks = []
    for label, spool in (("stdout", item.stdout_file), ("stderr", item.stderr_file)):
        try:
            text = Path(spool).read_text(encoding="utf-8", errors="replace").strip()
        except OSError:
            continue
        if text:
            chunks.append(f"[{label}]\n{text[-limit:]}")
    return "\n".join(chunks).strip() or None


def _format_stop(item: _Session) -> str:
    """Summarize process state, stop reason, location, threads, and output."""
    lldb = _lldb()
    process = item.process
    lines = [
        f"session: {item.id}",
        f"program: {item.program_label}",
        f"state: {lldb.SBDebugger.StateAsCString(process.GetState())}",
    ]
    if process.GetState() == lldb.eStateExited:
        lines.append(f"exit_status: {process.GetExitStatus()}")
    else:
        selected = process.GetSelectedThread()
        lines.append(f"stop: {_stop_reason(selected)}")
        frame = selected.GetSelectedFrame()
        lines.append(f"location: {_location(frame)}")
        for index in range(process.GetNumThreads()):
            thread = process.GetThreadAtIndex(index)
            marker = "*" if thread.GetIndexID() == selected.GetIndexID() else " "
            lines.append(
                f"{marker} thread {thread.GetIndexID()} "
                f"({thread.GetName() or 'unnamed'}): "
                f"{_location(thread.GetFrameAtIndex(0))}"
            )
        locals_lines = _frame_summary(frame, arguments_only=False, limit=15)
        lines.append("locals (frame #0):")
        if locals_lines:
            lines.extend(f"  {entry}" for entry in locals_lines)
        else:
            lines.append("  (no variables)")
    output = _inferior_output(item)
    if output is not None:
        lines.append(f"output:\n{output}")
    return "\n".join(lines)


def _value_text(value, depth: int = 0) -> str:
    """Render one SBValue with a one-level expansion for aggregates."""
    text = value.GetValue()
    if text is None:
        summary = value.GetSummary()
        text = summary if summary else f"<{value.GetTypeName()}>"
    if len(text) > _MAX_VALUE_CHARS:
        text = text[:_MAX_VALUE_CHARS] + "..."
    if value.GetValue() is None and depth < 1:
        try:
            count = value.GetNumChildren()
        except Exception:  # noqa: BLE001 - exotic types
            count = 0
        if count:
            parts = []
            for index in range(min(count, 8)):
                child = value.GetChildAtIndex(index)
                parts.append(f"{child.GetName()}={_value_text(child, depth + 1)}")
            text += " {" + ", ".join(parts) + ("..." if count > 8 else "") + "}"
    return text


def start(program: Path, args: list[str], stdin_text: str) -> str:
    """Create a session with the target loaded; launch later with run."""
    _lldb()
    validate_args(args)
    if len(stdin_text) > STDIN_MAX_CHARS:
        raise ValueError(f"stdin_text exceeds {STDIN_MAX_CHARS} characters")
    with _SESSIONS_LOCK:
        _sweep()
        if len(_SESSIONS) >= _MAX_SESSIONS:
            raise ValueError(f"Too many debug sessions (max {_MAX_SESSIONS}). Stop one first.")
        session_id = f"dbg-{uuid.uuid4().hex[:8]}"
        # Always spool stdin (empty file when no input): the inferior must
        # never inherit the server's stdio pipe, where a blocking read
        # would consume MCP protocol bytes and kill the connection.
        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".stdin", delete=False, encoding="utf-8"
        ) as spool:
            spool.write(stdin_text)
            stdin_file = spool.name
        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".stdout", delete=False, encoding="utf-8"
        ) as spool:
            stdout_file = spool.name
        with tempfile.NamedTemporaryFile(
            mode="w", suffix=".stderr", delete=False, encoding="utf-8"
        ) as spool:
            stderr_file = spool.name
        try:
            item = _Session(
                session_id, program, args, stdin_file, stdout_file, stderr_file
            )
        except Exception:
            for spool in (stdin_file, stdout_file, stderr_file):
                if spool is not None:
                    Path(spool).unlink(missing_ok=True)
            raise
        _SESSIONS[session_id] = item
    return f"session: {session_id}\nprogram: {item.program_label}\nstate: created"


def add_breakpoint(
    session_id: str, symbol: str | None, file: str | None, line: int | None
) -> str:
    """Add a symbol or file:line breakpoint to a session's target."""
    item = _get(session_id)
    if symbol and (file or line):
        raise ValueError("Pass symbol or file:line, not both.")
    if symbol:
        point = item.target.BreakpointCreateByName(symbol)
        label = symbol
    elif file and line:
        if line < 1:
            raise ValueError("line must be >= 1")
        point = item.target.BreakpointCreateByLocation(file, line)
        label = f"{file}:{line}"
    else:
        raise ValueError("Pass symbol or file and line.")
    if not point.IsValid():
        raise ValueError(f"Breakpoint rejected: {label}")
    if point.GetNumLocations() == 0:
        item.target.BreakpointDelete(point.GetID())
        raise ValueError(
            f"Breakpoint resolved to 0 locations: {label}. "
            "Use file and line for methods, or check the symbol with llvm tools."
        )
    item.breakpoints[point.GetID()] = label
    return (
        f"session: {session_id}\nbreakpoint {point.GetID()}: {label} "
        f"({point.GetNumLocations()} locations)"
    )


def launch(session_id: str, timeout: int = DEBUG_TIMEOUT_SECONDS) -> str:
    """Launch the inferior; blocks until it stops or exits."""
    item = _get(session_id)
    if item.live():
        raise ValueError(f"Session {session_id} already has a live inferior. Use debug_continue.")
    if item.launched:
        for spool in (item.stdout_file, item.stderr_file):
            try:
                Path(spool).write_text("", encoding="utf-8")
            except OSError:
                pass
    lldb = _lldb()
    error = lldb.SBError()

    def action():
        item.process = item.target.Launch(item._launch, error)
        return item.process

    _blocking(item, action, timeout)
    if not error.Success():
        raise ValueError(f"Launch failed: {error.GetCString()}")
    item.launched = True
    return _format_stop(item)


def proceed(session_id: str, timeout: int = DEBUG_TIMEOUT_SECONDS) -> str:
    """Continue a stopped inferior; blocks until it stops again or exits."""
    item = _get(session_id)
    if not item.live():
        raise ValueError(f"Session {session_id} has no live inferior. Use debug_run.")
    _blocking(item, item.process.Continue, timeout)
    return _format_stop(item)


def step(session_id: str, mode: str, timeout: int = DEBUG_TIMEOUT_SECONDS) -> str:
    """Step the selected thread: over, into, or out of the current frame."""
    item = _get(session_id)
    if not item.live():
        raise ValueError(f"Session {session_id} has no live inferior. Use debug_run.")
    thread = item.process.GetSelectedThread()
    if mode == "over":
        action = thread.StepOver
    elif mode == "into":
        action = thread.StepInto
    elif mode == "out":
        action = thread.StepOut
    else:
        raise ValueError(f"Unknown step mode: {mode}")
    _blocking(item, action, timeout)
    return _format_stop(item)


def where(session_id: str, depth: int = 10) -> str:
    """List threads plus the selected thread's backtrace with frame arguments."""
    item = _get(session_id)
    if not item.live():
        raise ValueError(f"Session {session_id} has no live inferior.")
    lines = [_format_stop(item)]
    selected = item.process.GetSelectedThread()
    lines.append(f"backtrace (thread {selected.GetIndexID()}):")
    for index in range(min(selected.GetNumFrames(), max(1, depth))):
        frame = selected.GetFrameAtIndex(index)
        lines.append(f"  #{index} {_location(frame)}")
        for entry in _frame_summary(frame, arguments_only=True, limit=8):
            lines.append(f"      arg {entry}")
    return "\n".join(lines)


def variables(session_id: str, frame: int = 0) -> str:
    """Show arguments and locals for one frame of the selected thread."""
    item = _get(session_id)
    if not item.live():
        raise ValueError(f"Session {session_id} has no live inferior.")
    selected = item.process.GetSelectedThread()
    if frame < 0 or frame >= selected.GetNumFrames():
        raise ValueError(f"Frame {frame} out of range (0..{selected.GetNumFrames() - 1}).")
    target = selected.GetFrameAtIndex(frame)
    lines = [f"session: {session_id}", f"frame #{frame}: {_location(target)}"]
    entries = _frame_summary(target, arguments_only=False, limit=_MAX_VARS)
    if entries:
        lines.extend(entries)
    else:
        lines.append("(no variables)")
    return "\n".join(lines)


def evaluate(session_id: str, expression: str, frame: int = 0) -> str:
    """Evaluate a C++ expression in one frame of the selected thread."""
    if not expression or len(expression) > 1024:
        raise ValueError("expression must hold 1..1024 characters")
    item = _get(session_id)
    if not item.live():
        raise ValueError(f"Session {session_id} has no live inferior.")
    selected = item.process.GetSelectedThread()
    if frame < 0 or frame >= selected.GetNumFrames():
        raise ValueError(f"Frame {frame} out of range (0..{selected.GetNumFrames() - 1}).")
    target = selected.GetFrameAtIndex(frame)
    value = _blocking(
        item, lambda: target.EvaluateExpression(expression), DEBUG_TIMEOUT_SECONDS
    )
    lines = [
        f"session: {session_id}",
        f"frame #{frame}: {_location(target)}",
        f"expression: {expression}",
    ]
    error = value.GetError()
    if error.Fail():
        lines.append(f"error: {error.GetCString()}")
    else:
        lines.append(f"result = {_value_text(value)} ({value.GetTypeName()})")
    return "\n".join(lines)


_MAX_COMMAND_CHARS = 512
_MAX_COMMAND_OUTPUT = 8000
# Verbs owned by session tools (debug_run/continue/break/stop) or able to
# escape the session (shell, script, quit, retargeting). Everything else,
# including help, disassemble, register, memory, thread, and expression,
# is available for manual calls.
_BLOCKED_VERBS = frozenset(
    {
        "run",
        "r",
        "continue",
        "c",
        "kill",
        "detach",
        "attach",
        "process",
        "platform",
        "script",
        "command",
        "breakpoint",
        "br",
        "b",
        "quit",
        "q",
        "exit",
        "target",
        "gui",
    }
)


def command(
    session_id: str, command_text: str, timeout: int = DEBUG_TIMEOUT_SECONDS
) -> str:
    """Run one raw lldb command in a session; `help` lists commands."""
    if not command_text or len(command_text) > _MAX_COMMAND_CHARS:
        raise ValueError(f"command must hold 1..{_MAX_COMMAND_CHARS} characters")
    verb = command_text.strip().split(None, 1)[0].lower()
    if verb in _BLOCKED_VERBS:
        raise ValueError(
            f"Command {verb!r} is owned by a session tool "
            "(debug_run/continue/break/stop) or would escape the session."
        )
    item = _get(session_id)
    lldb = _lldb()
    interpreter = item.debugger.GetCommandInterpreter()

    def action() -> tuple[bool, str]:
        result = lldb.SBCommandReturnObject()
        interpreter.HandleCommand(command_text, result)
        text = ((result.GetOutput() or "") + (result.GetError() or "")).strip()
        return (bool(result.Succeeded()), text or "(no output)")

    succeeded, text = _blocking(item, action, timeout)
    if len(text) > _MAX_COMMAND_OUTPUT:
        text = (
            text[:_MAX_COMMAND_OUTPUT]
            + f"\n... truncated ({len(text) - _MAX_COMMAND_OUTPUT} more characters)"
        )
    status = "success" if succeeded else "failure"
    return f"session: {session_id}\ncommand: {command_text}\nstatus: {status}\n{text}"


def stop(session_id: str) -> str:
    """Kill the inferior, destroy the debugger, forget the session."""
    with _SESSIONS_LOCK:
        item = _SESSIONS.pop(session_id, None)
    if item is None:
        raise ValueError(f"Unknown debug session: {session_id}.")
    program = item.program_label
    _destroy(item)
    return f"session: {session_id}\nprogram: {program}\nstate: stopped"
