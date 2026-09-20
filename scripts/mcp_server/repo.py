"""Repository path helpers: everything stays inside the worktree."""

from __future__ import annotations

from pathlib import Path

from .config import MAX_FILE_BYTES, ROOT


def inside_root(path: str) -> Path:
    """Resolve a repository-relative path and reject traversal outside the root."""
    candidate = (ROOT / path).resolve()
    if candidate != ROOT and ROOT not in candidate.parents:
        raise ValueError(f"Path escapes repository: {path}")
    return candidate


def relative_posix(path: Path) -> str:
    """Render an absolute path inside ROOT as a repository-relative string."""
    return path.resolve().relative_to(ROOT.resolve()).as_posix()


def read_text(path: str) -> str:
    """Read a bounded UTF-8 file from inside the repository."""
    candidate = inside_root(path)
    if not candidate.is_file():
        raise ValueError(f"Not a file: {path}")
    if candidate.stat().st_size > MAX_FILE_BYTES:
        raise ValueError(f"File exceeds {MAX_FILE_BYTES} bytes: {path}")
    return candidate.read_text(encoding="utf-8")
