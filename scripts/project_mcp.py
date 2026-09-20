#!/usr/bin/env python3
"""Launch the project MCP server over stdio (thin entry point)."""

from __future__ import annotations

import sys
from pathlib import Path

sys.path.insert(0, str(Path(__file__).resolve().parent))

from mcp_server.server import main

if __name__ == "__main__":
    main()
