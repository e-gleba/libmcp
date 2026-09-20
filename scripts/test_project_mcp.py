#!/usr/bin/env python3
"""Static smoke checks for project MCP without network or third-party imports."""

from __future__ import annotations

import ast
import json
import sys
from pathlib import Path

ROOT = Path(__file__).resolve().parent.parent
LAUNCHER = ROOT / "scripts" / "libmcp_project_mcp.py"
SERVER = ROOT / "scripts" / "project_mcp.py"
CONFIG = ROOT / "opencode.json"


def main() -> int:
    """Validate syntax and OpenCode launch contract."""
    for path in (LAUNCHER, SERVER):
        ast.parse(path.read_text(encoding="utf-8"), filename=str(path))

    config = json.loads(CONFIG.read_text(encoding="utf-8"))
    server = config["mcp"]["libmcp-project"]
    expected = ["python3", "scripts/libmcp_project_mcp.py"]
    if server.get("type") != "local" or server.get("command") != expected:
        raise AssertionError("OpenCode MCP command does not match launcher")
    if server.get("timeout", 0) < 30_000:
        raise AssertionError("OpenCode MCP startup timeout is too short")

    launcher = LAUNCHER.read_text(encoding="utf-8")
    if "os.execv" not in launcher or '"-u"' not in launcher:
        raise AssertionError("launcher must replace itself with unbuffered server")
    return 0


if __name__ == "__main__":
    sys.exit(main())
