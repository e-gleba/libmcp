"""CMake preset discovery: JSON preset files with inherits resolution."""

from __future__ import annotations

import json
from pathlib import Path

from .config import ROOT


def load_preset_file(
    preset_file: Path, seen: set[str]
) -> tuple[list[dict], list[dict]]:
    """Load configure/build presets, following CMake preset includes."""
    key = str(preset_file.resolve())
    if key in seen:
        return ([], [])
    seen.add(key)
    try:
        data = json.loads(preset_file.read_text(encoding="utf-8"))
    except (OSError, json.JSONDecodeError, UnicodeDecodeError):
        return ([], [])
    configure: list[dict] = list(data.get("configurePresets", []))
    build: list[dict] = list(data.get("buildPresets", []))
    for include in data.get("include", []):
        child = (preset_file.parent / str(include)).resolve()
        if child.is_file():
            child_configure, child_build = load_preset_file(child, seen)
            configure.extend(child_configure)
            build.extend(child_build)
    return (configure, build)


def expand_binary_dir(raw: str, preset_name: str) -> Path:
    """Expand ${sourceDir}/${presetName} and anchor relative paths at ROOT."""
    expanded = raw.replace("${sourceDir}", ROOT.as_posix()).replace(
        "${presetName}", preset_name
    )
    candidate = Path(expanded)
    if not candidate.is_absolute():
        candidate = ROOT / expanded
    return candidate


def configure_binary_dir(
    configure_map: dict[str, dict], name: str, original: str = "", depth: int = 0
) -> Path | None:
    """Resolve a configure preset's binaryDir, following inherits chains."""
    if depth > 10:
        return None
    preset = configure_map.get(name)
    if preset is None:
        return None
    raw = preset.get("binaryDir")
    if isinstance(raw, str) and raw:
        return expand_binary_dir(raw, original or name)
    inherits = preset.get("inherits", [])
    if isinstance(inherits, str):
        inherits = [inherits]
    for parent in inherits:
        resolved = configure_binary_dir(
            configure_map, str(parent), original or name, depth + 1
        )
        if resolved is not None:
            return resolved
    return None


def merge_build_preset(build_map: dict[str, dict], name: str) -> dict:
    """Merge a build preset with its inherits chain; child entries win."""
    merged: dict = {}
    seen: set[str] = set()

    def visit(current: str, depth: int) -> None:
        if depth > 10 or current in seen:
            return
        entry = build_map.get(current)
        if entry is None:
            return
        seen.add(current)
        inherits = entry.get("inherits", [])
        if isinstance(inherits, str):
            inherits = [inherits]
        for parent in inherits:
            visit(str(parent), depth + 1)
        merged.update(entry)

    visit(name, 0)
    return merged


def load_all() -> tuple[dict[str, dict], dict[str, dict]]:
    """Load every configure/build preset visible from the top-level file."""
    top = ROOT / "CMakePresets.json"
    configure_list, build_list = (
        load_preset_file(top, set()) if top.is_file() else ([], [])
    )
    configure_map = {
        str(item["name"]): item for item in configure_list if "name" in item
    }
    build_map = {str(item["name"]): item for item in build_list if "name" in item}
    return (configure_map, build_map)


def resolve_build_preset(build_preset: str) -> tuple[Path, str, str]:
    """Map a build preset to (binary_dir, configure_preset, configuration)."""
    configure_map, build_map = load_all()
    merged = merge_build_preset(build_map, build_preset)
    configure_name = merged.get("configurePreset")
    configuration = merged.get("configuration")
    if not isinstance(configure_name, str) or not configure_name:
        raise ValueError(
            f"Unknown build preset: {build_preset}. "
            "Call list_cmake_presets to see declared presets."
        )
    if not isinstance(configuration, str) or not configuration:
        raise ValueError(
            f"Build preset has no configuration: {build_preset}. "
            "Use a release/debug build preset."
        )
    binary_dir = configure_binary_dir(configure_map, configure_name)
    if binary_dir is None:
        raise ValueError(
            f"Cannot resolve binary directory for preset: {build_preset}."
        )
    return (binary_dir, configure_name, configuration)
