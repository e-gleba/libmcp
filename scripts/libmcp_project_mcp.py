#!/usr/bin/env python3
"""Launch project MCP with an isolated, reproducible Python environment."""

from __future__ import annotations

import hashlib
import os
import subprocess
import sys
from pathlib import Path

APP_NAME = "libmcp-project-mcp"
MCP_REQUIREMENT = "mcp==1.12.4"
MIN_PYTHON = (3, 10)
BOOTSTRAP_TIMEOUT_SECONDS = 15 * 60

SCRIPT_DIR = Path(__file__).resolve().parent
SERVER_SCRIPT = SCRIPT_DIR / "project_mcp.py"
CACHE_KEY = hashlib.sha256(MCP_REQUIREMENT.encode()).hexdigest()[:12]


def _fail(message: str) -> None:
    """Exit with actionable stderr while preserving MCP stdout."""
    print(f"{APP_NAME}: {message}", file=sys.stderr)
    raise SystemExit(1)


def _cache_root() -> Path:
    """Return platform cache directory without third-party dependencies."""
    if sys.platform == "win32":
        base = os.environ.get("LOCALAPPDATA")
        return (Path(base) if base else Path.home() / "AppData" / "Local") / "Cache"
    if sys.platform == "darwin":
        return Path.home() / "Library" / "Caches"
    xdg = os.environ.get("XDG_CACHE_HOME")
    return Path(xdg) if xdg else Path.home() / ".cache"


def _venv_python(venv: Path) -> Path:
    """Return virtual-environment interpreter path."""
    if sys.platform == "win32":
        return venv / "Scripts" / "python.exe"
    return venv / "bin" / "python"


def _run_bootstrap(command: list[str]) -> str | None:
    """Run bootstrap command; return diagnostic on failure."""
    try:
        result = subprocess.run(
            command,
            capture_output=True,
            text=True,
            timeout=BOOTSTRAP_TIMEOUT_SECONDS,
        )
    except subprocess.TimeoutExpired:
        return f"timed out after {BOOTSTRAP_TIMEOUT_SECONDS} seconds"
    except OSError as error:
        return str(error)

    if result.returncode == 0:
        return None
    return (result.stderr or result.stdout or f"exit code {result.returncode}").strip()


def _ensure_environment() -> Path:
    """Create or reuse versioned environment and return its interpreter."""
    if sys.version_info[:2] < MIN_PYTHON:
        _fail(
            f"Python {MIN_PYTHON[0]}.{MIN_PYTHON[1]}+ required; "
            f"found {sys.version.split()[0]}"
        )

    venv = _cache_root() / APP_NAME / CACHE_KEY
    python = _venv_python(venv)
    if python.is_file():
        return python

    error = _run_bootstrap([sys.executable, "-m", "venv", str(venv)])
    if error:
        _fail(f"could not create isolated environment: {error}")

    error = _run_bootstrap(
        [
            str(python),
            "-m",
            "pip",
            "install",
            "--quiet",
            "--disable-pip-version-check",
            MCP_REQUIREMENT,
        ]
    )
    if error:
        _fail(f"could not install {MCP_REQUIREMENT}: {error}")
    return python


def _check_server(python: Path) -> None:
    """Import server in isolated environment before OpenCode starts stdio."""
    error = _run_bootstrap([str(python), "-m", "py_compile", str(SERVER_SCRIPT)])
    if error:
        _fail(f"server validation failed: {error}")


def main() -> None:
    """Validate environment, then replace launcher process with MCP server."""
    if not SERVER_SCRIPT.is_file():
        _fail(f"missing server script: {SERVER_SCRIPT}")
    if sys.argv[1:] not in ([], ["--install-only"]):
        _fail(f"unknown argument: {sys.argv[1]}")

    python = _ensure_environment()
    _check_server(python)
    if sys.argv[1:] == ["--install-only"]:
        return

    try:
        os.execv(str(python), [str(python), "-u", str(SERVER_SCRIPT)])
    except OSError as error:
        _fail(f"could not launch MCP server: {error}")


if __name__ == "__main__":
    main()
