#!/usr/bin/env python3
"""
AbyssEngine / D.A.T.S Build System
Root-level. Incremental, multithreaded.
"""

import os
import sys
import shutil
import subprocess
import glob
import time
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

# ---------------------------------------------------------------
# Config
# ---------------------------------------------------------------

ROOT        = Path(__file__).parent.resolve()
SOURCE      = ROOT / "Source"
PARTY       = ROOT / "Party"
SHADERS     = ROOT / "Shaders"

VULKAN_SDK  = Path("C:/VulkanSDK/1.4.363.0")

CXX         = "g++"
CXX_FLAGS   = ["-std=c++17", "-O2", "-ffast-math"]

INCLUDES = [
    str(SOURCE / "Engine"),
    str(SOURCE / "UI"),
    str(SOURCE / "Game"),
    "C:/VulkanSDK/1.4.363.0/Include",
]

LIBS_WINDOWS = [
    "C:/VulkanSDK/1.4.363.0/Lib/vulkan-1.lib",
    "-lglfw3",
    "-lwebp",
    "-lgdi32",
    "-luser32",
    "-lshell32",
    "-lole32",
    "-lpthread",
]

LIBS_LINUX = [
    "-lvulkan",
    "-lglfw",
    "-lpthread",
    "-ldl",
    "-lm",
    "-lX11",
]

REPL = """
=================================
AbyssEngine/D.A.T.S BUILD SYSTEM
=================================

[ 1 ] Build (Full Wipe and Rebuild)
[ 2 ] Clear Build Directory
[ 3 ] Build (Incremental Build)
[ 4 ] Configuration
[ 5 ] Exit

"""

# ---------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------

def log(msg):
    print(f"[Compile] {msg}")

def run(cmd, cwd=None):
    result = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True)
    if result.returncode != 0:
        print(result.stdout)
        print(result.stderr)
        raise RuntimeError(f"Command failed: {' '.join(cmd)}")
    return result

def find_files(pattern, base):
    return sorted(glob.glob(str(base / pattern), recursive=True))

def get_libs(platform):
    if platform.lower() == "windows":
        return LIBS_WINDOWS
    elif platform.lower() == "linux":
        return LIBS_LINUX
    return LIBS_WINDOWS

def get_exe_name(platform):
    return "DATS.exe" if platform.lower() == "windows" else "DATS"

# ---------------------------------------------------------------
# Shaders
# ---------------------------------------------------------------

def build_shaders(build_dir):
    log("Building shaders...")
    out_dir = build_dir / "Shaders"
    out_dir.mkdir(parents=True, exist_ok=True)

    if not SHADERS.exists():
        log("  No Shaders/ folder, skipping.")
        return

    sources = find_files("*.vert", SHADERS) + find_files("*.frag", SHADERS)
    if not sources:
        log("  No shaders found.")
        return

    for f in sources:
        src = Path(f)
        out = out_dir / (src.name + ".spv")
        try:
            run(["glslangValidator", "-V", str(src), "-o", str(out)])
            log(f"  {src.name} -> {out.name}")
        except FileNotFoundError:
            log("  glslangValidator not found. Skipping shaders.")
            return

# ---------------------------------------------------------------
# Compile
# ---------------------------------------------------------------

def compile_one(src, obj, platform):
    cmd = [CXX] + CXX_FLAGS
    for inc in INCLUDES:
        cmd += ["-I", inc]
    cmd += ["-c", str(src), "-o", str(obj)]
    run(cmd)

def build(platform, full_wipe):
    build_dir = ROOT / "Build" / platform
    obj_dir = build_dir / "obj"

    if full_wipe and build_dir.exists():
        log(f"Wiping {build_dir}...")
        shutil.rmtree(build_dir)

    build_dir.mkdir(parents=True, exist_ok=True)
    obj_dir.mkdir(parents=True, exist_ok=True)

    # Shaders
    build_shaders(build_dir)

    # Sources
    cpp_files = find_files("**/*.cpp", SOURCE)
    if not cpp_files:
        log("No .cpp files found in Source/.")
        return

    log(f"Found {len(cpp_files)} source files.")

    jobs = []
    for src in cpp_files:
        src = Path(src)
        rel = src.relative_to(SOURCE)
        obj = obj_dir / (str(rel).replace("/", "_").replace("\\", "_") + ".o")
        jobs.append((src, obj))

    # Incremental check
    if full_wipe:
        to_build = jobs
    else:
        to_build = [
            (s, o) for (s, o) in jobs
            if not o.exists() or o.stat().st_mtime < s.stat().st_mtime
        ]

    if not to_build:
        log("Everything up to date.")
    else:
        log(f"Compiling {len(to_build)} files in parallel...")
        with ThreadPoolExecutor() as pool:
            futures = {pool.submit(compile_one, s, o, platform): s for s, o in to_build}
            for f in as_completed(futures):
                src = futures[f]
                try:
                    f.result()
                    log(f"  OK  {Path(src).name}")
                except Exception as e:
                    log(f"  FAIL {Path(src).name}")
                    raise

    # Link
    exe_name = get_exe_name(platform)
    out_exe = build_dir / exe_name

    log(f"Linking {exe_name}...")
    obj_files = [str(o) for _, o in jobs]
    link_cmd = [CXX] + obj_files + ["-o", str(out_exe)] + get_libs(platform)
    run(link_cmd)

    log(f"Build OK: {out_exe}")

# ---------------------------------------------------------------
# TUI
# ---------------------------------------------------------------

def main():
    while True:
        print(REPL)
        choice = input(">>> ").strip()
        print()

        if choice == "5":
            print("Goodbye.")
            break

        if choice == "1":
            platform = input("What is your Build Choice (Linux, Windows): ").strip()
            build(platform, full_wipe=True)
        elif choice == "2":
            build_root = ROOT / "Build"
            if build_root.exists():
                shutil.rmtree(build_root)
                print("Build directory cleared.")
            else:
                print("Build directory does not exist.")
        elif choice == "3":
            platform = input("What is your Build Choice (Linux, Windows): ").strip()
            build(platform, full_wipe=False)
        elif choice == "4":
            print("Configuration mode - not implemented.")
        else:
            print("Invalid choice, please try again.")

        print()

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        print("\nInterrupted.")
        sys.exit(1)
    except Exception as e:
        print(f"\nBUILD FAILED: {e}")
        input("Press Enter to exit...")