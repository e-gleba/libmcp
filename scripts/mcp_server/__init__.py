"""Project MCP server package: CMake-aware tools over stdio.

Reuse in any CMake project:

1. Copy ``scripts/mcp_server`` and ``scripts/project_mcp.py`` into the new
   repository (paths stay inside the worktree automatically via ROOT).
2. Optionally rename the server without editing code::

       PROJECT_MCP_SERVER_NAME=my-project  # FastMCP name, tool prefix
       PROJECT_MCP_APP_NAME=my-project-mcp  # isolated venv cache directory

3. Point the project's ``opencode.json`` at ``scripts/project_mcp.py``.
4. Requires the ``mcp`` SDK 1.x (``mcp<2``), CMake, and Ninja for target
   discovery; debuggers and LLVM tools are used only when installed.

No target, preset, or path names are hardcoded; everything is discovered
from ``CMakePresets.json`` and the CMake File API at runtime.
"""

from __future__ import annotations
