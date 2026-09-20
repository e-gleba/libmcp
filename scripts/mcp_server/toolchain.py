"""Native toolchain discovery: compilers, Visual Studio, shells, build tools.

Everything here is best-effort probing via PATH and well-known vendor tools;
nothing is executed beyond `--version`-free discovery (vswhere JSON output).
Safe to copy into any CMake project alongside the rest of this package.
"""

from __future__ import annotations

import json
import os
import platform
import shutil
import subprocess

COMPILERS = ("cc", "c++", "gcc", "g++", "clang", "clang++", "cl")
BUILD_TOOLS = ("cmake", "ninja", "make", "nasm")
SHELLS = ("pwsh", "powershell")
ANDROID_TOOLS = ("gradle", "adb", "java", "sdkmanager", "emulator")
ANDROID_ENV = (
    "ANDROID_HOME",
    "ANDROID_SDK_ROOT",
    "ANDROID_NDK_ROOT",
    "JAVA_HOME",
    "GRADLE_USER_HOME",
)


def which_all(names: tuple[str, ...]) -> list[tuple[str, str]]:
    """Resolve each tool name to its absolute path, skipping missing ones."""
    return [(name, path) for name in names if (path := shutil.which(name))]


def visual_studio() -> list[str]:
    """List installed Visual Studio instances via vswhere, if present."""
    vswhere = shutil.which("vswhere")
    if vswhere is None:
        return []
    try:
        result = subprocess.run(
            [
                vswhere,
                "-products",
                "*",
                "-requires",
                "Microsoft.VisualStudio.Component.VC.Tools",
                "-format",
                "json",
                "-utf8",
            ],
            check=False,
            capture_output=True,
            text=True,
            stdin=subprocess.DEVNULL,
            timeout=60,
        )
    except (OSError, subprocess.TimeoutExpired):
        return []
    if result.returncode != 0:
        return []
    try:
        installations = json.loads(result.stdout or "[]")
    except json.JSONDecodeError:
        return []
    lines: list[str] = []
    for item in installations:
        if not isinstance(item, dict):
            continue
        path = str(item.get("installationPath", ""))
        version = str(item.get("catalog", {}).get("productDisplayVersion", ""))
        label = f"{path} ({version})".strip() if path else ""
        if label:
            lines.append(label)
    return lines


def android_studio_dirs() -> list[str]:
    """Return existing Android Studio install directories for this platform."""
    home = os.path.expanduser("~")
    system = platform.system()
    if system == "Windows":
        program_files = os.environ.get("ProgramFiles", r"C:\Program Files")
        candidates = [os.path.join(program_files, "Android", "Android Studio")]
    elif system == "Darwin":
        candidates = ["/Applications/Android Studio.app"]
    else:
        candidates = [
            "/opt/android-studio",
            "/usr/local/android-studio",
            os.path.join(home, "android-studio"),
        ]
    studio_sh = shutil.which("studio.sh")
    found = [path for path in candidates if os.path.isdir(path)]
    if studio_sh:
        found.append(studio_sh)
    return found


def describe() -> str:
    """Summarize the host toolchain: platform, compilers, VS, shells, tools."""
    sections: list[str] = [f"platform: {platform.system()} {platform.machine()}"]
    compilers = which_all(COMPILERS)
    sections.append(
        "compilers:\n"
        + ("\n".join(f"  {name}: {path}" for name, path in compilers) if compilers else "  (none found)")
    )
    studios = visual_studio()
    sections.append(
        "visual_studio:\n"
        + ("\n".join(f"  {line}" for line in studios) if studios else "  (not found)")
    )
    shells = which_all(SHELLS)
    sections.append(
        "shells:\n"
        + ("\n".join(f"  {name}: {path}" for name, path in shells) if shells else "  (none found)")
    )
    build = which_all(BUILD_TOOLS)
    sections.append(
        "build_tools:\n"
        + ("\n".join(f"  {name}: {path}" for name, path in build) if build else "  (none found)")
    )
    android = which_all(ANDROID_TOOLS)
    sections.append(
        "android_tools:\n"
        + ("\n".join(f"  {name}: {path}" for name, path in android) if android else "  (none found)")
    )
    defined = [(name, os.environ[name]) for name in ANDROID_ENV if os.environ.get(name)]
    sections.append(
        "android_env:\n"
        + ("\n".join(f"  {name}={value}" for name, value in defined) if defined else "  (none set)")
    )
    studios_dirs = android_studio_dirs()
    sections.append(
        "android_studio:\n"
        + ("\n".join(f"  {path}" for path in studios_dirs) if studios_dirs else "  (not found)")
    )
    return "\n".join(sections)
