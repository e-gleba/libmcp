"""FastMCP application: resources, tools, and prompts for the repository."""

from __future__ import annotations

import json
import shutil
from typing import Annotated, Literal

from mcp.server.fastmcp import FastMCP
from pydantic import Field

from . import artifacts, debuggers, debug_sessions, llvm, mcp_client, presets, repo, toolchain
from .config import (
    DEBUG_TIMEOUT_SECONDS,
    PRESET_PATTERN,
    SERVER_NAME,
    STDIN_MAX_CHARS,
    TARGET_PATTERN,
)
from .process import run

mcp = FastMCP(
    SERVER_NAME,
    instructions=(
        "Use resources for project context before editing. Use tools for CMake "
        "discovery, configure, build, test, and debugging. All paths stay inside "
        "the repository."
    ),
)


@mcp.resource(
    "project://instructions",
    title="Agent instructions",
    description="Repository-specific instructions from AGENTS.md.",
    mime_type="text/markdown",
)
def instructions() -> str:
    """Return repository agent instructions."""
    return repo.read_text("AGENTS.md")


@mcp.resource(
    "project://readme",
    title="Project README",
    description="Project overview, supported platforms, and build examples.",
    mime_type="text/markdown",
)
def readme() -> str:
    """Return the project README."""
    return repo.read_text("readme.md")


@mcp.resource(
    "project://cmake-presets",
    title="CMake presets",
    description="Top-level CMake preset file.",
    mime_type="application/json",
)
def cmake_presets() -> str:
    """Return the top-level CMake presets file."""
    return repo.read_text("CMakePresets.json")


@mcp.tool()
def project_tree(
    path: Annotated[str, Field(description="Repository-relative directory")] = ".",
    depth: Annotated[int, Field(ge=0, le=5)] = 2,
) -> str:
    """List project files without reading their contents."""
    start = repo.inside_root(path)
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
def read_project_file(
    path: Annotated[str, Field(description="Repository-relative UTF-8 file")],
) -> str:
    """Read a bounded UTF-8 file from inside the repository."""
    return repo.read_text(path)


@mcp.tool()
def git_status() -> str:
    """Return machine-readable repository status without changing files."""
    return run(["git", "status", "--short", "--branch"])


@mcp.tool()
def list_cmake_presets() -> str:
    """Ask CMake for configure, build, test, package, and workflow presets."""
    return run(["cmake", "--list-presets=all"])


@mcp.tool()
def cmake_configure(
    preset: Annotated[str, Field(pattern=PRESET_PATTERN)],
    fresh: bool = False,
    timeout_seconds: Annotated[int, Field(ge=60, le=3600)] = 600,
) -> str:
    """Configure one declared CMake preset. First configures download
    toolchains and sources and can take a while; raise timeout_seconds
    (up to one hour) for large or uncached projects."""
    command = ["cmake", "--preset", preset]
    if fresh:
        command.append("--fresh")
    return run(command, timeout=timeout_seconds)


@mcp.tool()
def cmake_build(
    preset: Annotated[str, Field(pattern=PRESET_PATTERN)],
    target: Annotated[str | None, Field(pattern=TARGET_PATTERN)] = None,
    timeout_seconds: Annotated[int, Field(ge=60, le=3600)] = 1800,
) -> str:
    """Build one declared CMake build preset and optional target. Full and
    cold-cache builds of large projects can take a while; raise
    timeout_seconds (up to one hour) rather than retrying blindly."""
    command = ["cmake", "--build", "--preset", preset]
    if target:
        command.extend(["--target", target])
    return run(command, timeout=timeout_seconds)


@mcp.tool()
def clean_build(
    build_preset: Annotated[str, Field(pattern=PRESET_PATTERN)],
    target: Annotated[
        str | None, Field(pattern=TARGET_PATTERN)
    ] = None,
    timeout_seconds: Annotated[int, Field(ge=60, le=3600)] = 1800,
) -> str:
    """Fresh-configure (removes CMakeCache.txt and CMakeFiles/, recreates from
    scratch per cmake --fresh docs; FetchContent sources re-download) and then
    build one CMake build preset and optional target. Skips the build when
    the fresh configure fails. Timeouts apply per step."""
    _, configure_name, _ = presets.resolve_build_preset(build_preset)
    configured = run(
        ["cmake", "--preset", configure_name, "--fresh"],
        timeout=min(timeout_seconds, 900),
    )
    sections = [f"== configure --fresh ({configure_name}) ==\n{configured}"]
    if not configured.startswith("exit_code: 0"):
        sections.append("build skipped: fresh configure failed");
        return "\n".join(sections)
    command = ["cmake", "--build", "--preset", build_preset]
    if target:
        command.extend(["--target", target])
    built = run(command, timeout=timeout_seconds)
    sections.append(f"== build ({build_preset}) ==\n{built}")
    return "\n".join(sections)


@mcp.tool()
def ctest(
    preset: Annotated[str, Field(pattern=PRESET_PATTERN)],
    timeout_seconds: Annotated[int, Field(ge=60, le=3600)] = 900,
) -> str:
    """Run one declared CTest preset with failure output enabled. Device and
    integration suites can run long; raise timeout_seconds (up to one hour)
    instead of re-running a suite that is still going."""
    return run(
        ["ctest", "--preset", preset, "--output-on-failure"],
        timeout=timeout_seconds,
    )


@mcp.tool()
def list_configurations(
    build_preset: Annotated[str, Field(pattern=PRESET_PATTERN)],
) -> str:
    """List build configurations for a preset, marking the preset's own."""
    names, _, configure_name, configuration = artifacts.configurations(build_preset)
    if not names:
        raise ValueError(
            f"No configurations found for preset {build_preset}. "
            f"Configure first: cmake_configure preset={configure_name}."
        )
    lines = [
        f"{name} (selected)" if name == configuration else name for name in names
    ]
    return "\n".join(lines)


@mcp.tool()
def list_build_targets(
    build_preset: Annotated[str, Field(pattern=PRESET_PATTERN)],
) -> str:
    """List buildable targets with types for one CMake build preset."""
    names, _, configure_name, configuration = artifacts.targets(build_preset)
    if not names:
        raise ValueError(
            f"No targets found for build preset {build_preset}. "
            f"Configure first: cmake_configure preset={configure_name}."
        )
    lines = [
        f"{name} [{kind}]" if kind else name for name, kind in names
    ]
    footer = (
        f"\nbuild_preset: {build_preset}\nconfigure_preset: {configure_name}"
        f"\nconfiguration: {configuration}"
    )
    return "\n".join(lines) + footer


@mcp.tool()
def compile_database(
    build_preset: Annotated[str, Field(pattern=PRESET_PATTERN)],
) -> str:
    """Locate the compile_commands.json for a build preset (clangd input)."""
    binary_dir, configure_name, _ = presets.resolve_build_preset(build_preset)
    database = binary_dir / "compile_commands.json"
    if database.is_file():
        return repo.relative_posix(database)
    raise ValueError(
        f"No compile_commands.json for preset {build_preset}. "
        f"Configure first: cmake_configure preset={configure_name}."
    )


@mcp.tool()
def list_executables(
    build_preset: Annotated[str, Field(pattern=PRESET_PATTERN)],
) -> str:
    """List built executables for one declared CMake build preset."""
    programs, _, configuration = artifacts.executables(build_preset)
    if not programs:
        raise ValueError(
            f"No executables found for preset {build_preset} "
            f"({configuration}). Build first: cmake_build preset={build_preset}."
        )
    return "\n".join(repo.relative_posix(item) for item in programs)


@mcp.tool()
def list_libraries(
    build_preset: Annotated[str, Field(pattern=PRESET_PATTERN)],
) -> str:
    """List built shared and static libraries for one CMake build preset."""
    found, _, configuration = artifacts.libraries(build_preset)
    if not found:
        raise ValueError(
            f"No libraries found for preset {build_preset} "
            f"({configuration}). Build first: cmake_build preset={build_preset}."
        )
    return "\n".join(
        f"{repo.relative_posix(path)} [{kind}]" for path, kind in found
    )


@mcp.tool()
def available_debuggers() -> str:
    """List supported debuggers installed on this host."""
    from .config import DEBUGGER_ORDER

    lines = [
        f"{name}: {path}"
        for name in DEBUGGER_ORDER
        if (path := shutil.which(name))
    ]
    return "\n".join(lines) if lines else "No supported debugger found."


@mcp.tool()
def llvm_tools() -> str:
    """List installed LLVM and Clang tools with their paths."""
    found = llvm.discover()
    if not found:
        return "No LLVM or Clang tools found."
    return "\n".join(f"{name}: {path}" for name, path in found)


@mcp.tool()
def toolchain_info() -> str:
    """Describe the host toolchain: compilers, Visual Studio, shells, tools."""
    return toolchain.describe()


@mcp.tool()
def inspect_binary(
    build_preset: Annotated[str, Field(pattern=PRESET_PATTERN)],
    target: Annotated[str, Field(pattern=TARGET_PATTERN)],
) -> str:
    """Show headers and sections of a built target via llvm-readobj or file."""
    program = artifacts.resolve_binary(
        build_preset,
        target,
        artifacts.EXECUTABLE_KINDS | artifacts.LIBRARY_KINDS,
    )
    return llvm.inspect_binary(program)


@mcp.tool()
def debug_executable(
    executable: Annotated[str, Field(description="Repository-relative executable")],
    debugger: Literal["auto", "gdb", "lldb", "cdb"] = "auto",
    args: Annotated[
        list[str], Field(description="Program arguments, no shell", max_length=32)
    ] = [],
    breakpoints: Annotated[
        list[str],
        Field(description="Breakpoints: symbol or file:line", max_length=16),
    ] = [],
    timeout_seconds: Annotated[int, Field(ge=5, le=600)] = DEBUG_TIMEOUT_SECONDS,
    stdin_text: Annotated[
        str, Field(description="Standard input for the program", max_length=STDIN_MAX_CHARS)
    ] = "",
) -> str:
    """Run an executable under a debugger; capture stack, locals, threads."""
    debuggers.validate_breakpoints(breakpoints)
    program = repo.inside_root(executable)
    if not program.is_file():
        raise ValueError(f"Not a file: {executable}")
    return debuggers.run_program(
        program, debugger, args, breakpoints, timeout_seconds, stdin_text
    )


@mcp.tool()
def debug_target(
    build_preset: Annotated[str, Field(pattern=PRESET_PATTERN)],
    target: Annotated[str, Field(pattern=TARGET_PATTERN)],
    debugger: Literal["auto", "gdb", "lldb", "cdb"] = "auto",
    args: Annotated[
        list[str], Field(description="Program arguments, no shell", max_length=32)
    ] = [],
    breakpoints: Annotated[
        list[str],
        Field(description="Breakpoints: symbol or file:line", max_length=16),
    ] = [],
    timeout_seconds: Annotated[int, Field(ge=5, le=600)] = DEBUG_TIMEOUT_SECONDS,
    stdin_text: Annotated[
        str, Field(description="Standard input for the program", max_length=STDIN_MAX_CHARS)
    ] = "",
) -> str:
    """Resolve a CMake target to its executable, then debug it with backtrace."""
    program = artifacts.resolve_binary(build_preset, target)
    return debuggers.run_program(
        program, debugger, args, breakpoints, timeout_seconds, stdin_text
    )


@mcp.tool()
def debug_start(
    executable: Annotated[str, Field(description="Repository-relative executable")],
    args: Annotated[
        list[str], Field(description="Program arguments, no shell", max_length=32)
    ] = [],
    stdin_text: Annotated[
        str, Field(description="Standard input for the program", max_length=STDIN_MAX_CHARS)
    ] = "",
) -> str:
    """Start a stateful lldb session; launch later with debug_run."""
    program = repo.inside_root(executable)
    if not program.is_file():
        raise ValueError(f"Not a file: {executable}")
    return debug_sessions.start(program, args, stdin_text)


@mcp.tool()
def debug_help(
    debugger: Literal["auto", "gdb", "lldb", "cdb"] = "auto",
    topic: Annotated[str, Field(description="Help topic, empty for command list")] = "",
) -> str:
    """Ask the debugger for help: command list or one topic's manual."""
    return debuggers.help_text(debugger, topic)


@mcp.tool()
def debug_eval(
    session: Annotated[str, Field(description="Session id from debug_start")],
    expression: Annotated[str, Field(description="C++ expression to evaluate")],
    frame: Annotated[int, Field(ge=0)] = 0,
) -> str:
    """Evaluate a C++ expression in one frame of a debug session."""
    return debug_sessions.evaluate(session, expression, frame)


@mcp.tool()
def debug_break(
    session: Annotated[str, Field(description="Session id from debug_start")],
    symbol: Annotated[str | None, Field(description="Function or method name")] = None,
    file: Annotated[str | None, Field(description="Source file for file:line")] = None,
    line: Annotated[int | None, Field(ge=1)] = None,
) -> str:
    """Add a symbol or file:line breakpoint to a debug session."""
    return debug_sessions.add_breakpoint(session, symbol, file, line)


@mcp.tool()
def debug_run(
    session: Annotated[str, Field(description="Session id from debug_start")],
    timeout_seconds: Annotated[int, Field(ge=5, le=600)] = DEBUG_TIMEOUT_SECONDS,
) -> str:
    """Launch a session's inferior; blocks until it stops or exits."""
    return debug_sessions.launch(session, timeout_seconds)


@mcp.tool()
def debug_continue(
    session: Annotated[str, Field(description="Session id from debug_start")],
    timeout_seconds: Annotated[int, Field(ge=5, le=600)] = DEBUG_TIMEOUT_SECONDS,
) -> str:
    """Continue a stopped inferior until it stops again or exits."""
    return debug_sessions.proceed(session, timeout_seconds)


@mcp.tool()
def debug_step(
    session: Annotated[str, Field(description="Session id from debug_start")],
    mode: Literal["over", "into", "out"] = "over",
    timeout_seconds: Annotated[int, Field(ge=5, le=600)] = DEBUG_TIMEOUT_SECONDS,
) -> str:
    """Step the selected thread over, into, or out of the current frame."""
    return debug_sessions.step(session, mode, timeout_seconds)


@mcp.tool()
def debug_where(
    session: Annotated[str, Field(description="Session id from debug_start")],
    depth: Annotated[int, Field(ge=1, le=50)] = 10,
) -> str:
    """List session threads plus the selected thread's backtrace."""
    return debug_sessions.where(session, depth)


@mcp.tool()
def debug_vars(
    session: Annotated[str, Field(description="Session id from debug_start")],
    frame: Annotated[int, Field(ge=0)] = 0,
) -> str:
    """Show arguments and locals for one frame of the selected thread."""
    return debug_sessions.variables(session, frame)


@mcp.tool()
def debug_command(
    session: Annotated[str, Field(description="Session id from debug_start")],
    command: Annotated[
        str,
        Field(description="Raw lldb command; `help` lists commands", max_length=512),
    ],
    timeout_seconds: Annotated[int, Field(ge=5, le=600)] = DEBUG_TIMEOUT_SECONDS,
) -> str:
    """Run one raw lldb command in a session (disassemble, registers, help)."""
    return debug_sessions.command(session, command, timeout_seconds)


@mcp.tool()
def debug_stop(
    session: Annotated[str, Field(description="Session id from debug_start")],
) -> str:
    """Kill the inferior, destroy the debugger, forget the session."""
    return debug_sessions.stop(session)


@mcp.tool()
def mcp_probe(
    build_preset: Annotated[str, Field(pattern=PRESET_PATTERN)],
    target: Annotated[str, Field(pattern=TARGET_PATTERN)],
    call_tool: Annotated[str | None, Field(description="Tool to call after listing")] = None,
    call_arguments_json: Annotated[
        str, Field(description="JSON object with the tool arguments")
    ] = "{}",
    timeout_seconds: Annotated[int, Field(ge=5, le=600)] = 60,
) -> str:
    """Launch a built target as an MCP server over stdio; list and call tools."""
    program = artifacts.resolve_binary(
        build_preset, target, artifacts.EXECUTABLE_KINDS
    )
    try:
        arguments = json.loads(call_arguments_json)
    except json.JSONDecodeError as error:
        raise ValueError(f"call_arguments_json is not valid JSON: {error}") from error
    if not isinstance(arguments, dict):
        raise ValueError("call_arguments_json must hold a JSON object")
    return mcp_client.probe(program, call_tool, arguments, timeout_seconds)


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
def fix_cmake(task: str, preset: str) -> str:
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


@mcp.prompt()
def debug_workflow(target: str, build_preset: str) -> str:
    """Debug one CMake target as a five-step task with captured evidence."""
    return f"""Debug CMake target {target} with build preset {build_preset}.

Follow these task steps in order, no unrelated files:
0. On an unknown host, call toolchain_info once to confirm compilers,
   debuggers, and build tools before assuming anything.
1. Call list_build_targets for {build_preset} and confirm {target} exists.
2. Call list_executables (or list_libraries for a library target) to resolve
   the binary path. Use inspect_binary for a header/arch sanity check.
3. Build only the target: cmake_build preset={build_preset} target={target}.
4. Debug with debug_target preset={build_preset} target={target}, using
   breakpoints ["main"] first, then narrower file:line breakpoints on the
   suspect function. Pass program args explicitly, never through a shell.
   For stdin-driven programs, feed input via the stdin_text parameter.
5. Report the failing frame, locals, args, threads, and the next surgical fix.
Stop after step 4 if the program exits cleanly and summarize the clean run.

For long interactive sessions instead: debug_start, debug_break, debug_run,
then debug_step (over/into/out), debug_where, debug_vars, debug_continue,
debug_stop. One session stays alive across calls until debug_stop.
For manual control inside a session: debug_command with any raw lldb
command (`help` lists them; disassemble, register, memory, thread);
process control and shell escapes stay with the session tools."""


def main() -> None:
    """Run the MCP server over stdio."""
    # Main thread: pre-import lldb (signal handlers) so debug tool calls
    # later hit the cached fast path from worker threads. Best effort:
    # lldb is optional and tools report its absence clearly.
    debug_sessions.warmup()
    mcp.run()


if __name__ == "__main__":
    main()
