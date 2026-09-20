"""Shared constants and platform helpers for the project MCP server."""

from __future__ import annotations

import os
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent.parent

# Overridable without code edits when copying this package to another project.
SERVER_NAME = os.environ.get("PROJECT_MCP_SERVER_NAME", "libmcp-project")
MAX_FILE_BYTES = 256 * 1024
COMMAND_TIMEOUT_SECONDS = 15 * 60
DEBUG_TIMEOUT_SECONDS = 120
CODEMODEL_TIMEOUT_SECONDS = 20 * 60
STDIN_MAX_CHARS = 65536

PRESET_PATTERN = r"^[A-Za-z0-9_.+-]+$"
TARGET_PATTERN = r"^[A-Za-z0-9_.+:/-]+$"
BREAKPOINT_PATTERN = r"^[A-Za-z0-9_./:+-]+$"

IS_WINDOWS = sys.platform == "win32"

# Host-independent suffix sets: classify artifacts from any platform's build
# tree, even when the server runs elsewhere (Windows cross-builds on Linux).
ALL_EXECUTABLE_SUFFIXES = frozenset({".exe", ".bat", ".cmd"})
ALL_SHARED_SUFFIXES = frozenset({".so", ".dylib", ".dll"})
ALL_STATIC_SUFFIXES = frozenset({".a", ".lib"})
ALL_OBJECT_SUFFIXES = frozenset({".o", ".obj"})

DEBUGGER_ORDER = ("cdb", "gdb", "lldb") if IS_WINDOWS else ("gdb", "lldb", "cdb")

LLVM_TOOLS = (
    "clang",
    "clang++",
    "clang-format",
    "clang-tidy",
    "clangd",
    "lld",
    "llvm-cov",
    "llvm-nm",
    "llvm-objdump",
    "llvm-readobj",
    "llvm-symbolizer",
)
