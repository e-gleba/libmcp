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


def gdb_command(program: Path, args: list[str], breakpoints: list[str]) -> list[str]:
    """Build a non-interactive gdb session capturing stack, locals, threads."""
    command = ["gdb", "--batch", "-ex", "set pagination off"]
    for point in breakpoints:
        command.extend(["-ex", f"break {point}"])
    command.extend(
        [
            "-ex",
            "run",
            "-ex",
            "bt",
            "-ex",
            "info locals",
            "-ex",
            "info args",
            "-ex",
            "info threads",
            "-ex",
            "thread apply all bt full",
            "--args",
            str(program),
            *args,
        ]
    )
    return command


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
    return f"{header}\n{output}".rstrip()
