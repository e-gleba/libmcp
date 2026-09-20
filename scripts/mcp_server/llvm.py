"""LLVM and Clang tooling: discovery plus binary inspection."""

from __future__ import annotations

import shutil
from pathlib import Path

from . import repo as repo_paths
from .config import LLVM_TOOLS
from .process import capture
from .toolchain import which_all

INSPECT_TIMEOUT_SECONDS = 60
INSPECT_MAX_CHARS = 8000


def discover() -> list[tuple[str, str]]:
    """List installed LLVM/Clang tools with their absolute paths."""
    return which_all(LLVM_TOOLS)


def inspect_binary(program: Path) -> str:
    """Describe a built binary: format, architecture, headers, sections.

    Prefers llvm-readobj, falls back to llvm-objdump, then the `file`
    utility. Output is truncated to keep tool responses bounded.
    """
    label = repo_paths.relative_posix(program)
    if shutil.which("llvm-readobj"):
        status, output = capture(
            ["llvm-readobj", "--file-headers", "--sections", str(program)],
            timeout=INSPECT_TIMEOUT_SECONDS,
        )
        tool = "llvm-readobj --file-headers --sections"
    elif shutil.which("llvm-objdump"):
        status, output = capture(
            ["llvm-objdump", "--private-headers", str(program)],
            timeout=INSPECT_TIMEOUT_SECONDS,
        )
        tool = "llvm-objdump --private-headers"
    elif shutil.which("file"):
        status, output = capture(
            ["file", str(program)], timeout=INSPECT_TIMEOUT_SECONDS
        )
        tool = "file"
    else:
        return f"binary: {label}\nNo inspection tool installed (llvm-readobj, file)."
    if status != 0:
        return f"binary: {label}\ntool: {tool}\nexit_code: {status}\n{output}".rstrip()
    body = output[:INSPECT_MAX_CHARS]
    if len(output) > INSPECT_MAX_CHARS:
        body += f"\n... truncated ({len(output) - INSPECT_MAX_CHARS} more characters)"
    return f"binary: {label}\ntool: {tool}\n{body}".rstrip()
