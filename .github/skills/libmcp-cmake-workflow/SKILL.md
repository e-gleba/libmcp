---
name: libmcp-cmake-workflow
description: Configure, build, test, debug, or review this repository's CMake project. Use for CMakeLists, presets, toolchains, CPM dependencies, CTest, packaging, CI build failures, and cross-compilation work.
---

## Workflow

1. Read root `AGENTS.md` before planning changes.
2. Prefer `libmcp-project` MCP tools when available. Begin with `git_status`, `list_cmake_presets`, and only relevant `project_tree` or `read_project_file` calls.
3. State assumptions and success criteria. Ask when preset, platform, or expected behavior is unclear.
4. Reproduce failures before editing. For fixes, preserve failing output or add a focused test when practical.
5. Make smallest change that fixes requested behavior. Do not reformat or refactor adjacent files.
6. Use declared CMake presets. Never invent commands or manually recreate preset options.
7. Verify configure, build, and CTest for affected native preset. For preset, packaging, toolchain, or cross-platform changes, also verify relevant workflow or cross preset when environment supports it.
8. Report exact commands, exit codes, untested platforms, and remaining risks.

## CMake rules

- Target-based CMake only. No directory-wide compile flags, includes, links, or definitions.
- Explicit source lists. No `file(GLOB)`.
- Presets own build configuration and compiler selection.
- Generated files stay in binary directory.
- Keep `PUBLIC`, `PRIVATE`, and `INTERFACE` propagation minimal and correct.
- Do not add dependencies unless task requires them.

## Completion

Finish only when changed lines trace to task, generated artifacts are absent from diff, and available build/tests pass. If environment blocks verification, state exact missing dependency or platform instead of claiming success.
