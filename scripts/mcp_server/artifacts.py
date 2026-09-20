"""Build artifact discovery: executables and libraries, cross-platform."""

from __future__ import annotations

import os
import shutil
from pathlib import Path

from . import codemodel, presets
from . import repo as repo_paths
from .config import (
    ALL_EXECUTABLE_SUFFIXES,
    ALL_OBJECT_SUFFIXES,
)
from .process import capture

EXECUTABLE_KINDS = frozenset({"EXECUTABLE"})
LIBRARY_KINDS = frozenset({"SHARED_LIBRARY", "MODULE_LIBRARY", "STATIC_LIBRARY"})
IGNORED_TARGETS = frozenset(
    {
        "all",
        "clean",
        "help",
        "test",
        "package",
        "package_source",
        "install",
        "edit_cache",
        "rebuild_cache",
        "codegen",
    }
)


def _is_executable_file(path: Path) -> bool:
    """Check the executable bit on POSIX or a known suffix on Windows."""
    if path.suffix.lower() in ALL_EXECUTABLE_SUFFIXES:
        return True
    if path.suffix.lower() in ALL_OBJECT_SUFFIXES:
        return False
    return os.access(path, os.X_OK)


def classify(path: Path) -> str:
    """Classify a build output as executable, shared, static, or other.

    Suffix checks cover every supported platform (so Windows artifacts
    classify correctly when the server runs on Linux and vice versa);
    versioned names like libfoo.so.1.2 match via their full suffix list.
    """
    suffixes = {item.lower() for item in path.suffixes}
    if suffixes & ALL_SHARED_SUFFIXES:
        return "shared"
    if suffixes & ALL_STATIC_SUFFIXES:
        return "static"
    if suffixes & ALL_EXECUTABLE_SUFFIXES:
        return "executable"
    if _is_executable_file(path):
        return "executable"
    return "other"


def scan_config_dir(binary_dir: Path, configuration: str) -> list[Path]:
    """Fallback scan for executables when no File API reply exists.

    Matches per-configuration output directories first; when the generator
    lays outputs out flat (single-config), falls back to every executable
    outside dependency and bookkeeping directories.
    """
    if not binary_dir.is_dir():
        return []
    nested = [
        candidate
        for candidate in sorted(binary_dir.rglob("*"))
        if _scan_candidate(candidate, binary_dir, configuration)
    ]
    if nested:
        return nested
    return [
        candidate
        for candidate in sorted(binary_dir.rglob("*"))
        if _scan_candidate(candidate, binary_dir, "")
    ]


def _scan_candidate(candidate: Path, binary_dir: Path, configuration: str) -> bool:
    """Check one path for the fallback executable scan."""
    if "_deps" in candidate.parts or "CMakeFiles" in candidate.parts:
        return False
    if candidate.is_symlink() or not candidate.is_file():
        return False
    if configuration and configuration not in candidate.parts:
        return False
    if candidate.suffix.lower() in ALL_OBJECT_SUFFIXES:
        return False
    return _is_executable_file(candidate)


def ninja_targets(binary_dir: Path, configuration: str) -> list[str]:
    """Fallback target list via Ninja when the File API reply is missing."""
    ninja = shutil.which("ninja")
    ninja_file = binary_dir / f"build-{configuration}.ninja"
    if ninja is None or not ninja_file.is_file():
        return []
    status, output = capture(
        [ninja, "-f", ninja_file.name, "-t", "targets", "all"],
        timeout=60,
        workdir=binary_dir,
    )
    if status != 0:
        return []
    names: set[str] = set()
    for line in output.splitlines():
        target = line.split(":", 1)[0].strip()
        if not target or "/" in target or target in IGNORED_TARGETS:
            continue
        if target.startswith("cmake_object_order_depends_target_"):
            continue
        if target.startswith(codemodel.DASHBOARD_PREFIXES):
            continue
        if target.endswith((".a", ".so", ".lib", ".dll", ".dylib", ".txt", ".cmake", ".in", ".ninja")):
            continue
        names.add(target)
    return sorted(names)


def _artifact_paths(
    binary_dir: Path, entries: list[dict[str, object]], kinds: frozenset[str]
) -> list[Path]:
    """Resolve codemodel artifacts of the given kinds to absolute paths."""
    paths: list[Path] = []
    for entry in entries:
        if str(entry.get("type")) not in kinds:
            continue
        artifacts = entry.get("artifacts", [])
        if not isinstance(artifacts, list):
            continue
        for relative in artifacts:
            candidate = binary_dir / str(relative)
            if candidate.is_file():
                paths.append(candidate)
    return sorted(paths)


def executables(build_preset: str) -> tuple[list[Path], Path, str]:
    """List built executables for a build preset (codemodel first, scan fallback)."""
    entries, binary_dir, _, configuration = codemodel.query_targets(build_preset)
    programs = _artifact_paths(binary_dir, entries, EXECUTABLE_KINDS)
    if not programs:
        programs = scan_config_dir(binary_dir, configuration)
    return (programs, binary_dir, configuration)


def libraries(build_preset: str) -> tuple[list[tuple[Path, str]], Path, str]:
    """List built shared/static libraries for a build preset with their kinds."""
    entries, binary_dir, _, configuration = codemodel.query_targets(build_preset)
    found: list[tuple[Path, str]] = []
    for entry in entries:
        kind = str(entry.get("type"))
        if kind not in LIBRARY_KINDS:
            continue
        artifacts = entry.get("artifacts", [])
        if not isinstance(artifacts, list):
            continue
        for relative in artifacts:
            candidate = binary_dir / str(relative)
            if not candidate.is_file():
                continue
            found.append((candidate, classify(candidate)))
    return (sorted(found), binary_dir, configuration)


def configurations(build_preset: str) -> tuple[list[str], Path, str, str]:
    """List build configurations for a preset: File API first, cache fallback."""
    binary_dir, configure_name, configuration = presets.resolve_build_preset(
        build_preset
    )
    if codemodel.ensure_codemodel(binary_dir, configure_name):
        names = codemodel.configurations(binary_dir)
        if names:
            return (names, binary_dir, configure_name, configuration)
    names = codemodel.cache_configurations(binary_dir)
    return (names, binary_dir, configure_name, configuration)


def targets(build_preset: str) -> tuple[list[tuple[str, str | None]], Path, str, str]:
    """List buildable targets as (name, type) pairs for a build preset."""
    entries, binary_dir, configure_name, configuration = codemodel.query_targets(
        build_preset
    )
    if entries:
        named = sorted(
            {(str(item["name"]), str(item.get("type"))) for item in entries}
        )
        return (named, binary_dir, configure_name, configuration)
    fallback = [(name, None) for name in ninja_targets(binary_dir, configuration)]
    return (fallback, binary_dir, configure_name, configuration)


def _matching_paths(
    binary_dir: Path,
    entries: list[dict[str, object]],
    kinds: frozenset[str],
    target: str,
) -> tuple[list[Path], list[str]]:
    """Collect artifact paths matching a target name plus all known stems."""
    matches: list[Path] = []
    stems: set[str] = set()
    for entry in entries:
        if str(entry.get("type")) not in kinds:
            continue
        target_match = str(entry.get("name")) == target
        artifacts = entry.get("artifacts", [])
        if not isinstance(artifacts, list):
            continue
        for relative in artifacts:
            name = Path(str(relative)).name
            stems.add(Path(name).stem)
            if target_match or Path(name).stem == target or name == target:
                candidate = binary_dir / str(relative)
                matches.append(candidate)
    return (matches, sorted(stems))


def resolve_binary(
    build_preset: str, target: str, kinds: frozenset[str] = EXECUTABLE_KINDS
) -> Path:
    """Resolve a target name to its built binary for a build preset."""
    entries, binary_dir, _, configuration = codemodel.query_targets(build_preset)
    matches, available = _matching_paths(binary_dir, entries, kinds, target)
    if not matches and kinds == EXECUTABLE_KINDS:
        scanned = scan_config_dir(binary_dir, configuration)
        matches = [item for item in scanned if item.stem == target]
        if not available:
            available = sorted({item.stem for item in scanned})
    if not matches:
        known = ", ".join(available) if available else "(none built)"
        raise ValueError(
            f"Target {target!r} has no binary in preset {build_preset} "
            f"({configuration}). Available: {known}. "
            "Call list_build_targets or list_executables."
        )
    if len(matches) > 1:
        options = ", ".join(repo_paths.relative_posix(item) for item in matches)
        raise ValueError(
            f"Target {target!r} is ambiguous in preset {build_preset}: {options}. "
            "Call debug_executable with an explicit path."
        )
    program = matches[0]
    if not program.is_file():
        raise ValueError(
            f"Binary is missing: {repo_paths.relative_posix(program)}. "
            f"Build first: cmake_build preset={build_preset} target={target}."
        )
    return program
