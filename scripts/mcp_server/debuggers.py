"""Debugger launches: gdb/lldb/cdb sessions capturing stack and locals."""

from __future__ import annotations

import re
import shutil
import subprocess
from pathlib import Path

from . import repo as repo_paths
from .config import (
    BREAKPOINT_PATTERN,
    DEBUGGER_ORDER,
    DEBUG_TIMEOUT_SECONDS,
    ROOT,
    STDIN_MAX_CHARS,
)
from .process import decode_output, run


def select(requested: str) -> str:
    """Pick the requested debugger or the first installed one."""
    candidates = DEBUGGER_ORDER if requested == "auto" else (requested,)
    selected = next((name for name in candidates if shutil.which(name)), None)
    if selected is None:
        raise ValueError(f"Debugger not installed: {requested}")
    return selected


def validate_breakpoints(breakpoints: list[str]) -> None:
    """Reject malformed breakpoint specs before launching a debugger."""
    if len(breakpoints) > 16:
        raise ValueError("breakpoints must hold at most 16 items")
    for point in breakpoints:
        if not point or len(point) > 256:
            raise ValueError(f"Invalid breakpoint length: {point!r}")
        if re.fullmatch(BREAKPOINT_PATTERN, point) is None:
            raise ValueError(f"Invalid breakpoint: {point!r}")


def validate_args(args: list[str]) -> None:
    """Reject oversized argument lists; values never pass through a shell."""
    if len(args) > 32 or any(not item or len(item) > 512 for item in args):
        raise ValueError("args must hold at most 32 items of 1..512 characters")


def _mi(command: str) -> list[str]:
    """Wrap one gdb MI command for batch execution via interpreter-exec."""
    return ["-ex", f"interpreter-exec mi {command}"]


def gdb_command(program: Path, args: list[str], breakpoints: list[str]) -> list[str]:
    """Build a non-interactive gdb session querying machine-readable records.

    Breakpoints and run use the console interpreter; stack, arguments,
    variables, and threads come back as MI records (^done with
    bkpt/stack/threads/variables payloads) instead of scraped text.
    """
    command = ["gdb", "--batch", "-ex", "set pagination off"]
    for point in breakpoints:
        command.extend(_mi(f'"-break-insert {point}"'))
    command.append("-ex")
    command.append("run")
    command.extend(_mi('"-stack-list-frames"'))
    command.extend(_mi('"-stack-list-arguments 1"'))
    command.extend(_mi('"-stack-list-variables --frame 0 1"'))
    command.extend(_mi('"-thread-info"'))
    command.extend(["--args", str(program), *args])
    return command


def _mi_value(text: str, pos: int) -> tuple[object, int]:
    """Parse one MI value (string, tuple, or list); return (value, pos)."""
    while pos < len(text) and text[pos] in " ,":
        pos += 1
    if pos >= len(text):
        raise ValueError("truncated MI value")
    char = text[pos]
    if char == '"':
        out: list[str] = []
        pos += 1
        while True:
            if pos >= len(text):
                raise ValueError("unterminated MI string")
            char = text[pos]
            if char == "\\":
                nxt = text[pos + 1] if pos + 1 < len(text) else ""
                out.append({"n": "\n", "t": "\t", "r": "\r"}.get(nxt, nxt))
                pos += 2
            elif char == '"':
                return ("".join(out), pos + 1)
            else:
                out.append(char)
                pos += 1
    if char == "{":
        obj: dict[str, object] = {}
        pos += 1
        while True:
            while pos < len(text) and text[pos] in " ,":
                pos += 1
            if pos < len(text) and text[pos] == "}":
                return (obj, pos + 1)
            start = pos
            while pos < len(text) and text[pos] not in "=,}":
                pos += 1
            key = text[start:pos]
            if pos >= len(text) or text[pos] != "=":
                raise ValueError(f"bad MI tuple near {key!r}")
            value, pos = _mi_value(text, pos + 1)
            obj[key] = value
    if char == "[":
        items: list[object] = []
        pos += 1
        while True:
            while pos < len(text) and text[pos] in " ,":
                pos += 1
            if pos < len(text) and text[pos] == "]":
                return (items, pos + 1)
            probe = pos
            while probe < len(text) and text[probe] not in '=,}]"':
                probe += 1
            key = text[pos:probe]
            if (
                probe < len(text)
                and text[probe] == "="
                and key
                and all(part.isidentifier() for part in key.replace("-", "_").split())
            ):
                value, pos = _mi_value(text, probe + 1)
                items.append({key: value})
            else:
                value, pos = _mi_value(text, pos)
                items.append(value)
    raise ValueError(f"bad MI value at {pos}: {text[pos:pos + 20]!r}")


def _mi_record(line: str) -> tuple[str, dict[str, object]] | None:
    """Parse one `^done,...` or `*stopped,...` line into (kind, fields)."""
    for prefix, kind in (("^done,", "done"), ("*stopped,", "stopped")):
        if line.startswith(prefix):
            fields: dict[str, object] = {}
            rest = line[len(prefix) :]
            try:
                while True:
                    rest = rest.strip(" ,")
                    if not rest:
                        break
                    match = re.match(r"([A-Za-z][\w-]*)=", rest)
                    if not match:
                        break
                    value, end = _mi_value(rest, match.end())
                    fields[match.group(1)] = value
                    rest = rest[end:]
            except ValueError:
                return None
            return (kind, fields)
    return None


def _mi_location(frame: object) -> str:
    """Render one MI frame dict as `func at file:line [addr]`."""
    if not isinstance(frame, dict):
        return "??"
    name = str(frame.get("func", "??"))
    addr = f" [{frame['addr']}]" if frame.get("addr") else ""
    file = frame.get("file")
    line = frame.get("line")
    if file and line:
        return f"{name} at {Path(str(file)).name}:{line}{addr}"
    return f"{name}{addr}"


def _mi_bound(text: str, limit: int = 8000) -> str:
    """Truncate one rendered section to keep tool responses bounded."""
    if len(text) > limit:
        text = text[:limit] + f"\n... truncated ({len(text) - limit} more characters)"
    return text


def _render_mi(output: str) -> str | None:
    """Render gdb MI records as clean sections; None when nothing parsed."""
    stops: list[dict[str, object]] = []
    dones: list[dict[str, object]] = []
    for line in output.splitlines():
        parsed = _mi_record(line.strip())
        if parsed is None:
            continue
        kind, fields = parsed
        (stops if kind == "stopped" else dones).append(fields)
    stacks: list[dict[str, object]] = []
    record_stack = next((r.get("stack") for r in dones if "stack" in r), None)
    if isinstance(record_stack, list):
        for entry in record_stack:
            if isinstance(entry, dict):
                frame = entry.get("frame", entry)
                if isinstance(frame, dict):
                    stacks.append(frame)
    threads: list[dict[str, object]] = []
    for record in dones:
        payload = record.get("threads")
        if isinstance(payload, list):
            threads = [entry for entry in payload if isinstance(entry, dict)]
    variables: list[dict[str, object]] = []
    for record in dones:
        payload = record.get("variables")
        if isinstance(payload, list):
            variables = [entry for entry in payload if isinstance(entry, dict)]
    arguments: dict[str, list[dict[str, object]]] = {}
    for record in dones:
        payload = record.get("stack-args")
        if isinstance(payload, list):
            for entry in payload:
                if isinstance(entry, dict) and isinstance(entry.get("frame"), dict):
                    frame = entry["frame"]
                    arguments[str(frame.get("level", "?"))] = [
                        item for item in frame.get("args", []) if isinstance(item, dict)
                    ]
    if not stacks and not threads:
        return None
    lines: list[str] = []
    for record in dones:
        point = record.get("bkpt")
        if not isinstance(point, dict):
            continue
        lines.append(
            f"breakpoint {point.get('number', '?')}: "
            f"{point.get('func', '?')} at "
            f"{Path(str(point.get('file', '?'))).name}:{point.get('line', '?')} "
            f"[{point.get('addr', '?')}]"
        )
    current = next(
        (
            str(record.get("current-thread-id"))
            for record in dones
            if "current-thread-id" in record
        ),
        None,
    )
    if stops:
        stop = stops[-1]
        lines.append(
            f"stop: {stop.get('reason', '?')} "
            f"(thread {stop.get('thread-id', '?')}): "
            f"{_mi_location(stop.get('frame'))}"
        )
    elif current is not None:
        top = next(
            (
                entry.get("frame")
                for entry in threads
                if str(entry.get("id")) == current
            ),
            None,
        )
        state = next(
            (
                str(entry.get("state", "?"))
                for entry in threads
                if str(entry.get("id")) == current
            ),
            "?",
        )
        lines.append(f"stop: {state} (thread {current}): {_mi_location(top)}")
    if threads:
        lines.append("threads:")
        for entry in threads:
            marker = (
                "*" if current is not None and str(entry.get("id")) == current else " "
            )
            lines.append(
                f"{marker} [{entry.get('id', '?')}] {entry.get('name', '?')} "
                f"({entry.get('state', '?')}): {_mi_location(entry.get('frame'))}"
            )
    if stacks:
        lines.append(f"frames (thread {current or '?'}):")
        for entry in stacks:
            level = str(entry.get("level", "?"))
            lines.append(f"#{level} {_mi_location(entry)}")
            for item in arguments.get(level, []):
                lines.append(
                    f"    arg {item.get('name', '?')} = {item.get('value', '?')}"
                )
    if variables:
        lines.append("locals (frame #0):")
        for item in variables:
            lines.append(f"{item.get('name', '?')} = {item.get('value', '?')}")
    return _mi_bound("\n".join(lines))


def lldb_command(program: Path, args: list[str], breakpoints: list[str]) -> list[str]:
    """Build a non-interactive lldb session capturing frames and threads."""
    command = ["lldb", "--batch"]
    for point in breakpoints:
        head, _, tail = point.rpartition(":")
        if head and tail.isdigit():
            command.extend(["-o", f"breakpoint set --file {head} --line {tail}"])
        else:
            command.extend(["-o", f"breakpoint set --name {point}"])
    command.extend(
        [
            "-o",
            "run",
            "-o",
            "thread backtrace all",
            "-o",
            "frame variable",
            "-o",
            "thread list",
            "--",
            str(program),
            *args,
        ]
    )
    return command


def help_text(debugger: str, topic: str) -> str:
    """Ask the debugger for help: command list or one topic's manual."""
    if len(topic) > 128:
        raise ValueError("topic exceeds 128 characters")
    selected = select(debugger)
    if selected == "gdb":
        command = ["gdb", "--batch"]
        if topic:
            command.extend(["-ex", f"help {topic}"])
        else:
            command.extend(["-ex", "help"])
    elif selected == "lldb":
        command = ["lldb", "--batch", "-o", f"help {topic}" if topic else "help"]
    else:
        command = ["cdb", "-c", ".help;q"]
    output = run(command, timeout=60)
    lines = output.splitlines()
    if len(lines) > 120:
        lines = lines[:120] + [f"... truncated ({len(lines) - 120} more lines)"]
    return "\n".join(lines).rstrip()


def run_program(
    program: Path,
    debugger: str,
    args: list[str],
    breakpoints: list[str],
    timeout: int = DEBUG_TIMEOUT_SECONDS,
    stdin_text: str = "",
) -> str:
    """Run an executable under a debugger and return stack/locals/threads.

    stdin_text feeds the program's standard input (bounded); empty means
    immediate end-of-file so stdin-driven servers exit instead of hanging.
    """
    validate_args(args)
    validate_breakpoints(breakpoints)
    if len(stdin_text) > STDIN_MAX_CHARS:
        raise ValueError(f"stdin_text exceeds {STDIN_MAX_CHARS} characters")
    selected = select(debugger)
    if selected == "gdb":
        command = gdb_command(program, args, breakpoints)
    elif selected == "lldb":
        command = lldb_command(program, args, breakpoints)
    else:
        command = ["cdb", "-c", "g;~*k;q", str(program), *args]
    program_label = repo_paths.relative_posix(program)
    breakpoint_label = ", ".join(breakpoints) if breakpoints else "(none)"
    try:
        result = subprocess.run(
            command,
            cwd=ROOT,
            check=False,
            capture_output=True,
            text=True,
            input=stdin_text if stdin_text else None,
            stdin=subprocess.DEVNULL if not stdin_text else None,
            timeout=timeout,
        )
    except subprocess.TimeoutExpired as error:
        output = "\n".join(
            part
            for part in (decode_output(error.stdout), decode_output(error.stderr))
            if part
        )
        suffix = f"\n{output}" if output else ""
        return (
            f"exit_code: timeout\nprogram: {program_label}"
            f"\ndebugger: {selected}\ncommand exceeded {timeout} seconds" + suffix
        )
    except OSError as error:
        return f"exit_code: launch_error\n{error}"
    output = "\n".join(
        part
        for part in (decode_output(result.stdout), decode_output(result.stderr))
        if part
    )
    header = (
        f"exit_code: {result.returncode}\nprogram: {program_label}"
        f"\ndebugger: {selected}\nbreakpoints: {breakpoint_label}"
    )
    if selected == "gdb":
        body = _render_mi(output) or output
    else:
        body = output
    return f"{header}\n{body}".rstrip()
