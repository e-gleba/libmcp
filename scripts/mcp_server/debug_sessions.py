"""Long-lived lldb debug sessions: launch, breakpoints, stepping, inspection.

Unlike the one-shot batch tools in debuggers.py, sessions keep a live
inferior across tool calls: debug_start creates the session, debug_run
launches, debug_step/debug_continue advance it, debug_where/debug_vars
inspect it, debug_stop ends it. All blocking debugger calls run in a worker
thread with a timeout; a hung inferior is killed instead of hanging the MCP
call. Requires lldb with its Python API (found via `lldb -P`).
"""

from __future__ import annotations

import shutil
import subprocess
import sys
import tempfile
import threading
import uuid
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
    """Import the lldb API via the path reported by the lldb driver."""
    global _LLDB, _LLDB_ERROR
    if _LLDB is not None:
        return _LLDB
    if _LLDB_ERROR:
        raise ValueError(f"lldb Python API unavailable: {_LLDB_ERROR}")
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


class _Session:
    """One live debugger, target, and inferior plus stdin spool bookkeeping."""

    def __init__(
        self,
        session_id: str,
        program: Path,
        args: list[str],
        stdin_file: str | None,
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
        self._launch = launch
        self.process = None
        self.breakpoints: dict[int, str] = {}
        self.stdin_file = stdin_file

    def live(self) -> bool:
        """Check whether the inferior is currently running or stopped."""
        if self.process is None or not self.process.IsValid():
            return False
        return self.process.GetState() not in _dead_states()


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
    """Destroy debuggers of dead sessions to free slots."""
    dead = [key for key, item in _SESSIONS.items() if not item.live()]
    for key in dead:
        item = _SESSIONS.pop(key)
        _destroy(item)


def _destroy(item: _Session) -> None:
    """Kill the inferior, remove the stdin spool, destroy the debugger."""
    try:
        if item.process is not None and item.process.IsValid():
            item.process.Kill()
    except Exception:  # noqa: BLE001 - best-effort teardown
        pass
    if item.stdin_file is not None:
        Path(item.stdin_file).unlink(missing_ok=True)
    _lldb().SBDebugger.Destroy(item.debugger)


def _get(session_id: str) -> _Session:
    """Look up a session or raise a helpful error."""
    with _SESSIONS_LOCK:
        item = _SESSIONS.get(session_id)
    if item is None:
        known = ", ".join(sorted(_SESSIONS)) if _SESSIONS else "(none)"
        raise ValueError(f"Unknown debug session: {session_id}. Active: {known}.")
    return item


def _blocking(item: _Session, action, timeout: int):
    """Run a blocking debugger call; kill the inferior on timeout."""
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
            if item.process is not None and item.process.IsValid():
                item.process.Kill()
        except Exception:  # noqa: BLE001 - best-effort teardown
            pass
        worker.join(10)
        raise TimeoutError(
            f"debugger call exceeded {timeout} seconds; inferior killed"
        )
    if "error" in outcome:
        raise RuntimeError(outcome["error"])
    return outcome.get("value")


def _location(frame) -> str:
    """Render function, file, and line for a stack frame."""
    name = frame.GetFunctionName() or "??"
    entry = frame.GetLineEntry()
    if entry.IsValid():
        return f"{name} at {entry.GetFileSpec().GetFilename()}:{entry.GetLine()}"
    return name


def _stop_reason(thread) -> str:
    """Describe why a thread stopped, falling back to the numeric reason."""
    try:
        text = thread.GetStopDescription(256)
    except Exception:  # noqa: BLE001 - older bindings differ
        text = ""
    return text or f"reason={thread.GetStopReason()}"


def _format_stop(item: _Session) -> str:
    """Summarize process state, stop reason, location, and threads."""
    lldb = _lldb()
    process = item.process
    lines = [
        f"session: {item.id}",
        f"program: {item.program_label}",
        f"state: {lldb.SBDebugger.StateAsCString(process.GetState())}",
    ]
    if process.GetState() == lldb.eStateExited:
        lines.append(f"exit_status: {process.GetExitStatus()}")
        return "\n".join(lines)
    selected = process.GetSelectedThread()
    lines.append(f"stop: {_stop_reason(selected)}")
    lines.append(f"location: {_location(selected.GetSelectedFrame())}")
    for index in range(process.GetNumThreads()):
        thread = process.GetThreadAtIndex(index)
        marker = "*" if thread.GetIndexID() == selected.GetIndexID() else " "
        lines.append(
            f"{marker} thread {thread.GetIndexID()} "
            f"({thread.GetName() or 'unnamed'}): "
            f"{_location(thread.GetFrameAtIndex(0))}"
        )
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
        stdin_file = None
        if stdin_text:
            with tempfile.NamedTemporaryFile(
                mode="w", suffix=".stdin", delete=False, encoding="utf-8"
            ) as spool:
                spool.write(stdin_text)
                stdin_file = spool.name
        try:
            item = _Session(session_id, program, args, stdin_file)
        except Exception:
            if stdin_file is not None:
                Path(stdin_file).unlink(missing_ok=True)
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
    lldb = _lldb()
    error = lldb.SBError()

    def action():
        item.process = item.target.Launch(item._launch, error)
        return item.process

    _blocking(item, action, timeout)
    if not error.Success():
        raise ValueError(f"Launch failed: {error.GetCString()}")
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
    """List threads plus the selected thread's backtrace."""
    item = _get(session_id)
    if not item.live():
        raise ValueError(f"Session {session_id} has no live inferior.")
    lines = [_format_stop(item)]
    selected = item.process.GetSelectedThread()
    lines.append(f"backtrace (thread {selected.GetIndexID()}):")
    for index in range(min(selected.GetNumFrames(), max(1, depth))):
        lines.append(f"  #{index} {_location(selected.GetFrameAtIndex(index))}")
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
    shown = 0
    for value in target.GetVariables(True, True, False, True):
        if shown >= _MAX_VARS:
            lines.append(f"... truncated at {_MAX_VARS} variables")
            break
        lines.append(f"{value.GetName()} = {_value_text(value)} ({value.GetTypeName()})")
        shown += 1
    if not shown:
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
    value = target.EvaluateExpression(expression)
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


def stop(session_id: str) -> str:
    """Kill the inferior, destroy the debugger, forget the session."""
    with _SESSIONS_LOCK:
        item = _SESSIONS.pop(session_id, None)
    if item is None:
        raise ValueError(f"Unknown debug session: {session_id}.")
    program = item.program_label
    _destroy(item)
    return f"session: {session_id}\nprogram: {program}\nstate: stopped"
