"""CMake File API access: typed targets and artifacts without text scraping."""

from __future__ import annotations

import json
from pathlib import Path

from . import presets
from .config import CODEMODEL_TIMEOUT_SECONDS
from .process import capture

QUERY_NAME = "codemodel-v2"
DASHBOARD_PREFIXES = ("Continuous", "Nightly", "Experimental")


def _reply_dir(binary_dir: Path) -> Path:
    """Return the File API reply directory for a configured build tree."""
    return binary_dir / ".cmake" / "api" / "v1" / "reply"


def _newest_codemodel(reply: Path) -> Path | None:
    """Pick the newest codemodel reply file, if any client has queried it."""
    candidates = list(reply.glob("codemodel-v2-*.json"))
    if not candidates:
        return None
    return max(candidates, key=lambda item: item.stat().st_mtime)


def ensure_codemodel(binary_dir: Path, configure_preset: str) -> bool:
    """Write a codemodel query and reconfigure so the reply exists.

    Leaves the query file in place, so every later configure refreshes the
    reply automatically. Returns True when a reply file is available.
    """
    reply = _reply_dir(binary_dir)
    if _newest_codemodel(reply) is not None:
        return True
    query = binary_dir / ".cmake" / "api" / "v1" / "query" / QUERY_NAME
    try:
        query.parent.mkdir(parents=True, exist_ok=True)
        query.touch(exist_ok=True)
    except OSError:
        return False
    status, _ = capture(
        ["cmake", "--preset", configure_preset],
        timeout=CODEMODEL_TIMEOUT_SECONDS,
    )
    return status == 0 and _newest_codemodel(reply) is not None


def configurations(binary_dir: Path) -> list[str]:
    """List configurations present in the File API reply, if any."""
    newest = _newest_codemodel(_reply_dir(binary_dir))
    if newest is None:
        return []
    try:
        index = json.loads(newest.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError, UnicodeDecodeError):
        return []
    return [
        str(item["name"])
        for item in index.get("configurations", [])
        if isinstance(item, dict) and "name" in item
    ]


def cache_configurations(binary_dir: Path) -> list[str]:
    """Read configurations from CMakeCache: multi-config types or build type."""
    cache = binary_dir / "CMakeCache.txt"
    if not cache.is_file():
        return []
    multi: list[str] = []
    single = ""
    try:
        for line in cache.read_text(encoding="utf-8").splitlines():
            if line.startswith("CMAKE_CONFIGURATION_TYPES:"):
                multi = line.split("=", 1)[1].split(";") if "=" in line else []
            elif line.startswith("CMAKE_BUILD_TYPE:"):
                single = line.split("=", 1)[1].strip() if "=" in line else ""
    except (OSError, UnicodeDecodeError):
        return []
    names = [item for item in multi if item] or ([single] if single else [])
    return names


def read_codemodel(
    binary_dir: Path, configuration: str
) -> list[dict[str, object]]:
    """Read typed targets for one configuration from the File API reply.

    Each entry holds name, type (EXECUTABLE, SHARED_LIBRARY, ...), and a list
    of artifact paths relative to the build tree. Dashboard scripting targets
    are excluded. Returns an empty list when no reply is available.
    """
    newest = _newest_codemodel(_reply_dir(binary_dir))
    if newest is None:
        return []
    try:
        index = json.loads(newest.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError, UnicodeDecodeError):
        return []
    configurations = index.get("configurations", [])
    selected = next(
        (item for item in configurations if item.get("name") == configuration),
        configurations[0] if configurations else None,
    )
    if selected is None:
        return []
    targets: list[dict[str, object]] = []
    for entry in selected.get("targets", []):
        name = str(entry.get("name", ""))
        json_file = entry.get("jsonFile")
        if not name or not isinstance(json_file, str):
            continue
        if name.startswith(DASHBOARD_PREFIXES):
            continue
        target_file = newest.parent / json_file
        try:
            details = json.loads(target_file.read_text(encoding="utf-8"))
        except (OSError, json.JSONDecodeError, UnicodeDecodeError):
            continue
        artifacts = [
            str(item["path"])
            for item in details.get("artifacts", [])
            if isinstance(item, dict) and "path" in item
        ]
        targets.append(
            {"name": name, "type": str(details.get("type", "UNKNOWN")), "artifacts": artifacts}
        )
    return targets


def query_targets(build_preset: str) -> tuple[list[dict[str, object]], Path, str, str]:
    """Resolve a build preset and return its codemodel targets.

    Configures on demand when no reply exists yet. Returns (targets,
    binary_dir, configure_preset, configuration); targets may be empty when
    CMake is missing or the tree was never configured.
    """
    binary_dir, configure_name, configuration = presets.resolve_build_preset(
        build_preset
    )
    if not ensure_codemodel(binary_dir, configure_name):
        return ([], binary_dir, configure_name, configuration)
    return (
        read_codemodel(binary_dir, configuration),
        binary_dir,
        configure_name,
        configuration,
    )
