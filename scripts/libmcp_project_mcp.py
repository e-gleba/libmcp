#!/usr/bin/env python3
"""Bootstrap an isolated environment and launch the project MCP server.

The official MCP Python SDK requires Python 3.10+. This launcher keeps every
dependency inside a per-user cache directory so the host Python's site-packages
are never modified. The MCP stdio protocol requires stdout to stay clean, so all
bootstrap output is captured and only failures are written to stderr.
"""

from __future__ import annotations

import os
import subprocess
import sys
from pathlib import Path

APP_NAME = "libmcp-project-mcp"
MCP_PIN = "mcp==1.12.4"
MIN_PYTHON = (3, 10)

SCRIPT_DIR = Path(__file__).resolve().parent
SERVER_SCRIPT = SCRIPT_DIR / "project_mcp.py"


def _fail(message: str) -> None:
    """Write a fatal error to stderr and exit, keeping stdout clean for MCP."""
    print(f"{APP_NAME}: {message}", file=sys.stderr)
    sys.exit(1)


def _cache_root() -> Path:
    """Return the platform cache directory without third-party helpers."""
    if sys.platform == "win32":
        base = os.environ.get("LOCALAPPDATA")
        return (Path(base) if base else Path.home() / "AppData" / "Local") / "Cache"
    if sys.platform == "darwin":
        return Path.home() / "Library" / "Caches"
    xdg = os.environ.get("XDG_CACHE_HOME")
    return Path(xdg) if xdg else Path.home() / ".cache"


def _venv_python(venv: Path) -> Path:
    """Return the interpreter path for an isolated virtual environment."""
    if sys.platform == "win32":
        return venv / "Scripts" / "python.exe"
    return venv / "bin" / "python"


def _run_captured(command: list[str]) -> str:
    """Run a bootstrap command silently; return diagnostics only on failure."""
    result = subprocess.run(command, capture_output=True, text=True)
    if result.returncode == 0:
        return ""
    return (result.stderr or result.stdout or f"exit code {result.returncode}").strip()


def _ensure_environment() -> Path:
    """Create or reuse the isolated environment and return its interpreter."""
    if sys.version_info[:2] < MIN_PYTHON:
        _fail(
            f"requires Python {MIN_PYTHON[0]}.{MIN_PYTHON[1]}+ "
            f"(the official MCP SDK does not support older); "
            f"found {sys.version.split()[0]}"
        )

    env_root = _cache_root() / APP_NAME
    venv = env_root / "venv"
    python = _venv_python(venv)
    stamp = env_root / "requirements.stamp"

    env_root.mkdir(parents=True, exist_ok=True)

    if not python.is_file():
        error = _run_captured([sys.executable, "-m", "venv", str(venv)])
        if error:
            _fail(f"could not create isolated environment: {error}")

    if not stamp.is_file() or stamp.read_text(encoding="utf-8").strip() != MCP_PIN:
        error = _run_captured(
            [
                str(python),
                "-m",
                "pip",
                "install",
                "--quiet",
                "--disable-pip-version-check",
                MCP_PIN,
            ]
        )
        if error:
            _fail(f"could not install the MCP SDK: {error}")
        stamp.write_text(MCP_PIN + "\n", encoding="utf-8")

    return python


def main() -> None:
    """Ensure the isolated environment, then run the server over stdio."""
    if not SERVER_SCRIPT.is_file():
        _fail(f"missing server script {SERVER_SCRIPT}")

    python = _ensure_environment()

    if "--install-only" in sys.argv[1:]:
        return

    completed = subprocess.run([str(python), str(SERVER_SCRIPT)])
    sys.exit(completed.returncode)


if __name__ == "__main__":
    main()