#!/usr/bin/env python3
"""MCP server exposing repository context and safe project commands."""

from __future__ import annotations

import subprocess
from pathlib import Path
from typing import Annotated, Literal

from mcp.server.fastmcp import FastMCP
from pydantic import Field

ROOT = Path(__file__).resolve().parent.parent
MAX_FILE_BYTES = 256 * 1024
COMMAND_TIMEOUT_SECONDS = 15 * 60

mcp = FastMCP(
    "libmcp-project",
    instructions=(
        "Use resources for project context before editing. Use tools for CMake "
        "discovery, configure, build, and tests. All paths stay inside the repository."
    ),
)


def _inside_root(path: str) -> Path:
    """Resolve a repository-relative path and reject traversal outside the root."""
    candidate = (ROOT / path).resolve()
    if candidate != ROOT and ROOT not in candidate.parents:
        raise ValueError(f"Path escapes repository: {path}")
    return candidate


def _read_text(path: str) -> str:
    """Read a bounded UTF-8 file from inside the repository."""
    candidate = _inside_root(path)
    if not candidate.is_file():
        raise ValueError(f"Not a file: {path}")
    if candidate.stat().st_size > MAX_FILE_BYTES:
        raise ValueError(f"File exceeds {MAX_FILE_BYTES} bytes: {path}")
    return candidate.read_text(encoding="utf-8")


def _run(command: list[str]) -> str:
    """Run a fixed command without a shell and return bounded-time diagnostics."""
    try:
        result = subprocess.run(  # noqa: S603 - argv uses fixed executables and validated args.
            command,
            cwd=ROOT,
            check=False,
            capture_output=True,
            text=True,
            timeout=COMMAND_TIMEOUT_SECONDS,
        )
    except subprocess.TimeoutExpired as error:
        output = "\n".join(
            part.rstrip()
            for part in (error.stdout or "", error.stderr or "")
            if part
        )
        return (
            f"exit_code: timeout\ncommand exceeded {COMMAND_TIMEOUT_SECONDS} seconds"
            f"{f'\n{output}' if output else ''}"
        )

    output = "\n".join(
        part for part in (result.stdout.rstrip(), result.stderr.rstrip()) if part
    )
    return f"exit_code: {result.returncode}\n{output}".rstrip()


@mcp.resource(
    "project://instructions",
    title="Agent instructions",
    description="Repository-specific instructions from AGENTS.md.",
    mime_type="text/markdown",
)
def instructions() -> str:
    """Return repository agent instructions."""
    return _read_text("AGENTS.md")


@mcp.resource(
    "project://readme",
    title="Project README",
    description="Project overview, supported platforms, and build examples.",
    mime_type="text/markdown",
)
def readme() -> str:
    """Return the project README."""
    return _read_text("readme.md")


@mcp.resource(
    "project://cmake-presets",
    title="CMake presets",
    description="Top-level CMake preset file.",
    mime_type="application/json",
)
def cmake_presets() -> str:
    """Return the top-level CMake presets file."""
    return _read_text("CMakePresets.json")


@mcp.resource(
    "project://file/{path}",
    title="Repository file",
    description="UTF-8 project file, restricted to the repository root.",
    mime_type="text/plain",
)
def project_file(path: str) -> str:
    """Return a bounded repository file."""
    return _read_text(path)


@mcp.tool()
def project_tree(
    path: Annotated[str, Field(description="Repository-relative directory")] = ".",
    depth: Annotated[int, Field(ge=0, le=5)] = 2,
) -> str:
    """List project files without reading their contents."""
    start = _inside_root(path)
    if not start.is_dir():
        raise ValueError(f"Not a directory: {path}")

    ignored = {".git", ".venv", "__pycache__", "build"}
    lines: list[str] = []
    for candidate in sorted(start.rglob("*")):
        relative = candidate.relative_to(start)
        if any(part in ignored or part.startswith("build-") for part in relative.parts):
            continue
        if len(relative.parts) > depth:
            continue
        suffix = "/" if candidate.is_dir() else ""
        lines.append(f"{relative.as_posix()}{suffix}")
    return "\n".join(lines)


@mcp.tool()
def list_cmake_presets() -> str:
    """Ask CMake for configure, build, test, package, and workflow presets."""
    return _run(["cmake", "--list-presets=all"])


@mcp.tool()
def cmake_configure(
    preset: Annotated[str, Field(pattern=r"^[A-Za-z0-9_.+-]+$")],
    fresh: bool = False,
) -> str:
    """Configure one declared CMake preset."""
    command = ["cmake", "--preset", preset]
    if fresh:
        command.append("--fresh")
    return _run(command)


@mcp.tool()
def cmake_build(
    preset: Annotated[str, Field(pattern=r"^[A-Za-z0-9_.+-]+$")],
    target: Annotated[
        str | None, Field(pattern=r"^[A-Za-z0-9_.+:/-]+$")
    ] = None,
) -> str:
    """Build one declared CMake build preset and optional target."""
    command = ["cmake", "--build", "--preset", preset]
    if target:
        command.extend(["--target", target])
    return _run(command)


@mcp.tool()
def ctest(
    preset: Annotated[str, Field(pattern=r"^[A-Za-z0-9_.+-]+$")],
) -> str:
    """Run one declared CTest preset with failure output enabled."""
    return _run(["ctest", "--preset", preset, "--output-on-failure"])


@mcp.tool()
def git_status() -> str:
    """Return machine-readable repository status without changing files."""
    return _run(["git", "status", "--short", "--branch"])


@mcp.tool()
def read_project_file(
    path: Annotated[str, Field(description="Repository-relative UTF-8 file")],
) -> str:
    """Read a bounded UTF-8 file from inside the repository."""
    return _read_text(path)


@mcp.prompt()
def inspect_project(task: str) -> str:
    """Gather project context before planning or editing."""
    return f"""You are working on this repository.

Task: {task}

Before proposing changes:
1. Read project://instructions and project://readme.
2. Read project://cmake-presets when the task touches builds, tests, packaging, or CI.
3. Call project_tree only for directories relevant to the task.
4. Read the exact files you intend to change.
5. State assumptions, a minimal plan, and verification commands.
Do not edit unrelated files."""


@mcp.prompt()
def fix_cmake(task: str, preset: str = "dev") -> str:
    """Investigate a CMake failure using repository conventions."""
    return f"""Fix this CMake problem with a surgical diff:

{task}

Use preset: {preset}

Read project://instructions and project://cmake-presets first. Inspect relevant
CMake files, reproduce the failure, fix only its cause, then configure, build,
and run tests using declared presets. Report commands and exit codes."""


@mcp.prompt()
def review_change(scope: Literal["working-tree", "staged"] = "working-tree") -> str:
    """Review a local change against project rules."""
    return f"""Review the {scope} changes. Read project://instructions first.
Check correctness, cross-platform CMake behavior, tests, accidental scope growth,
and generated/build artifacts. Report actionable findings by severity with file
and line references. If no findings exist, say so and list verification gaps."""


def main() -> None:
    """Run the MCP server over stdio."""
    mcp.run()


if __name__ == "__main__":
    main()
