#!/usr/bin/env python3
"""
Abyss Engine / D.A.T.S build System
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
SOURCE      = ROOT / "source"
INCLUDE     = ROOT / "include"
LIB_DIR     = ROOT / "lib"
WREN_SRC    = ROOT / "compiler" / "src"
build_ROOT  = ROOT / "build"
SHADERS     = ROOT / "Shaders"  # Lowercase 's'
VULKAN_ROOT = ROOT / "vulkansdk"
VULKAN_INC  = VULKAN_ROOT / "multiarch" / "include"
VULKAN_LIB  = VULKAN_ROOT / "windows" / "lib"

# ---------------------------------------------------------------
# C++ Config
# ---------------------------------------------------------------

CXX         = "g++"
CXX_FLAGS   = [
    "-std=c++17",
    "-O2",
    "-I", str(INCLUDE),
    "-I", str(VULKAN_INC),
    "-I", str(WREN_SRC / "include"),
    "-I", str(WREN_SRC / "vm"),
    "-I", str(ROOT / "openxr" / "include"),
]

CPP_FILES = []

C_FILES = [
    str(WREN_SRC / "vm" / f) for f in [
        "wren_compiler.c", "wren_core.c", "wren_debug.c",
        "wren_primitive.c", "wren_utils.c", "wren_value.c", "wren_vm.c"
    ]
] + [
    str(WREN_SRC / "optional" / "wren_opt_meta.c"),
    str(WREN_SRC / "optional" / "wren_opt_random.c"),
]

# Engine sources
ENGINE_CPP = [
    str(SOURCE / "vkrender.cpp"),
    str(SOURCE / "engine.cpp"),
    str(SOURCE / "main.cpp"),
]

LIBS_WINDOWS = [
    str(VULKAN_LIB / "vulkan-1.lib"),
    "-L", str(LIB_DIR),
    "-lglfw3",
    "-lgdi32", "-luser32", "-lshell32", "-lole32",
]

LIBS_LINUX = [
    "-lvulkan",
    "-lglfw",
    "-lpthread", "-ldl", "-lm", "-lX11",
]

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

def get_libs(platform):
    if platform.lower() == "linux":
        return LIBS_LINUX
    return LIBS_WINDOWS

def get_exe_name(platform):
    return "DATS.exe" if platform.lower() == "windows" else "DATS"

def get_platform_dir(platform):
    return build_ROOT / platform.capitalize()

# ---------------------------------------------------------------
# Shaders (Using glslc)
# ---------------------------------------------------------------

def build_shaders(build_dir):
    log("building shaders with glslc...")
    out_dir = build_dir / "Shaders"
    out_dir.mkdir(parents=True, exist_ok=True)

    if not SHADERS.exists():
        log("  No Shaders/ folder, skipping.")
        return

    sources = []
    for ext in ("*.vert", "*.frag"):
        sources += glob.glob(str(SHADERS / ext))

    if not sources:
        log("  No shaders found.")
        return

    # Point to glslc.exe instead of glslangValidator.exe
    glslc = VULKAN_ROOT / "windows" / "bin" / "glslc.exe"
    if not glslc.exists():
        log("  glslc.exe not found, skipping shaders.")
        return

    for f in sources:
        src = Path(f)
        out = out_dir / (src.name + ".spv")
        
        # glslc uses different arguments than glslangValidator
        # -fshader-stage is optional if extension is .vert/.frag, but we'll be explicit
        stage = "vert" if src.suffix == ".vert" else "frag"
        
        cmd = [str(glslc), f"-fshader-stage={stage}", str(src), "-o", str(out)]
        run(cmd)
        log(f"  {src.name} -> {out.name}")

# ---------------------------------------------------------------
# Compile
# ---------------------------------------------------------------

def compile_one(src, obj, platform, extra_flags):
    cmd = [CXX] + CXX_FLAGS + extra_flags
    cmd += ["-c", str(src), "-o", str(obj)]
    run(cmd)

def build(platform, full_wipe):
    build_dir = get_platform_dir(platform)
    obj_dir   = build_dir / "obj"

    if full_wipe and build_dir.exists():
        log(f"Wiping {build_dir}...")
        shutil.rmtree(build_dir)

    build_dir.mkdir(parents=True, exist_ok=True)
    obj_dir.mkdir(parents=True, exist_ok=True)

    # Shaders
    build_shaders(build_dir)

    # Compile C files
    log(f"Compiling {len(C_FILES)} C files...")
    c_objs = []
    for src in C_FILES:
        srcp = Path(src)
        obj = obj_dir / (srcp.name + ".o")
        if not full_wipe and obj.exists() and obj.stat().st_mtime >= srcp.stat().st_mtime:
            c_objs.append(obj)
            continue
        cmd = ["gcc", "-std=c99", "-O2", "-c", str(srcp),
               "-I", str(WREN_SRC / "include"),
               "-I", str(WREN_SRC / "vm"),
               "-I", str(WREN_SRC / "optional"),
               "-o", str(obj)]
        run(cmd)
        log(f"  {srcp.name}")
        c_objs.append(obj)

    # Compile C++ files
    cpp_files = ENGINE_CPP + CPP_FILES
    log(f"Compiling {len(cpp_files)} C++ files...")
    cpp_objs = []
    for src in cpp_files:
        srcp = Path(src)
        if not srcp.exists():
            log(f"  MISSING: {src}")
            continue
        obj = obj_dir / (srcp.name + ".o")
        if not full_wipe and obj.exists() and obj.stat().st_mtime >= srcp.stat().st_mtime:
            cpp_objs.append(obj)
            continue
        cmd = [CXX] + CXX_FLAGS + ["-c", str(srcp), "-o", str(obj)]
        result = subprocess.run(cmd, capture_output=True, text=True)
        if result.returncode != 0:
            log(f"  FAIL: {srcp.name}")
            print(result.stderr)
            raise RuntimeError(f"Failed to compile {srcp.name}")
        log(f"  OK: {srcp.name}")
        cpp_objs.append(obj)

    # Link
    exe_name = get_exe_name(platform)
    out_exe  = build_dir / exe_name
    log(f"Linking {exe_name}...")

    all_objs = [str(o) for o in c_objs + cpp_objs]
    link_cmd = [CXX] + all_objs + ["-o", str(out_exe)] + get_libs(platform)
    result = subprocess.run(link_cmd, capture_output=True, text=True)
    if result.returncode != 0:
        log("LINKER ERROR:")
        print(result.stderr)
        raise RuntimeError("Failed to link executable")

    log(f"build OK: {out_exe}")
    return out_exe

# ---------------------------------------------------------------
# TUI
# ---------------------------------------------------------------

REPL = """
=================================
AbyssEngine/D.A.T.S build SYSTEM
=================================

[ 1 ] Build (Full Wipe and Rebuild)
[ 2 ] Clear build Directory
[ 3 ] Build (Incremental Build)
[ 4 ] Configuration
[ 5 ] Exit

"""

def main():
    while True:
        print(REPL)
        choice = input(">>> ").strip()
        print()

        if choice == "5":
            print("Goodbye.")
            break

        if choice == "1":
            platform = input("What is your build Choice (Linux, Windows): ").strip()
            try:
                exe = build(platform, full_wipe=True)
                print(f"\nLaunching {exe}...")
                subprocess.run([str(exe)], cwd=str(exe.parent))
            except Exception as e:
                print(f"\nbuild FAILED: {e}")
        elif choice == "2":
            if build_ROOT.exists():
                shutil.rmtree(build_ROOT)
                print("build directory cleared.")
            else:
                print("build directory does not exist.")
        elif choice == "3":
            platform = input("What is your build Choice (Linux, Windows): ").strip()
            try:
                exe = build(platform, full_wipe=False)
                print(f"\nLaunching {exe}...")
                subprocess.run([str(exe)], cwd=str(exe.parent))
            except Exception as e:
                print(f"\nbuild FAILED: {e}")
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
        print(f"\nbuild FAILED: {e}")
        sys.exit(1)