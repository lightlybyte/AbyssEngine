#!/usr/bin/env python3
"""
Abyss Engine / D.A.T.S build System
Root-level. Incremental, multithreaded. MinGW-compatible on Windows.
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
BUILD_ROOT  = ROOT / "build"
SHADERS     = ROOT / "Shaders"
VULKAN_ROOT = ROOT / "vulkansdk"

# SDK layout is: vulkansdk/windows/{Bin,Lib,Include} and multiarch/include.
# Prefer a real SDK layout if present; fall back to the lowercase layout.
def _first_existing(*paths):
    for p in paths:
        if Path(p).exists():
            return Path(p)
    return Path(paths[-1])

VULKAN_INC = _first_existing(
    VULKAN_ROOT / "multiarch" / "include",
    VULKAN_ROOT / "Multiarch" / "Include",
    VULKAN_ROOT / "include",
)
VULKAN_LIB = _first_existing(
    VULKAN_ROOT / "windows" / "Lib",
    VULKAN_ROOT / "windows" / "lib",
    VULKAN_ROOT / "Lib",
)
VULKAN_BIN = _first_existing(
    VULKAN_ROOT / "windows" / "Bin",
    VULKAN_ROOT / "windows" / "bin",
    VULKAN_ROOT / "Bin",
)

# ---------------------------------------------------------------
# C++ Config
# ---------------------------------------------------------------

CXX       = "g++"
CC        = "gcc"
CXX_FLAGS = [
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
    str(SOURCE / "python.cpp"),
]

# Python embedding config
# Prefer environment-driven discovery so the build is not machine-locked.
def _find_python_root():
    env = os.environ.get("DATS_PYTHON_ROOT")
    if env:
        p = Path(env)
        if p.exists():
            return p
    candidates = [
        Path(r"C:\Users\lightlybyt\AppData\Local\Python\pythoncore-3.12-64"),
        Path(r"C:\Python312"),
        Path(r"C:\Program Files\Python312"),
    ]
    for c in candidates:
        if c.exists():
            return c
    return None

PYTHON_SYSTEM_ROOT = _find_python_root()
if PYTHON_SYSTEM_ROOT is not None:
    PYTHON_INC = PYTHON_SYSTEM_ROOT / "Include"
    PYTHON_LIB = PYTHON_SYSTEM_ROOT / "libs"
    PYTHON_DLL = PYTHON_SYSTEM_ROOT / "python312.dll"
    if PYTHON_INC.exists():
        CXX_FLAGS += ["-I", str(PYTHON_INC)]
    else:
        print(f"[Compile] WARNING: Python Include dir not found: {PYTHON_INC}")
else:
    PYTHON_INC = PYTHON_LIB = PYTHON_DLL = None
    print("[Compile] WARNING: Python system root not found; python.cpp will likely fail to compile.")

# Local embeddable package copied next to the executable at runtime (no env vars needed)
PYTHON_EMBED_ROOT = LIB_DIR / "pythonembed"

# Windows link line. g++ / MinGW wants GNU import libs (-lXXX), NOT MSVC .lib files.
LIBS_WINDOWS = [
    "-L", str(VULKAN_LIB),
    "-lvulkan-1",
    "-L", str(PYTHON_LIB) if PYTHON_LIB else "",
    "-lpython312",
    "-L", str(LIB_DIR),
    "-lglfw3",
    "-lgdi32", "-luser32", "-lshell32", "-lole32",
]
# strip the empty entries that come from a missing Python lib
LIBS_WINDOWS = [x for x in LIBS_WINDOWS if x]

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
    result = subprocess.run(
        cmd, cwd=cwd, capture_output=True, text=True, errors="replace"
    )
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
    return BUILD_ROOT / platform.capitalize()

# ---------------------------------------------------------------
# Shaders (Using glslc)
# ---------------------------------------------------------------

def build_shaders(build_dir, platform):
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

    # Pick glslc based on target platform. Windows uses VULKAN_BIN; Linux uses PATH.
    if platform.lower() == "windows":
        glslc = VULKAN_BIN / "glslc.exe"
        if not glslc.exists():
            log(f"  glslc.exe not found in {VULKAN_BIN}, skipping shaders.")
            return
        glslc_cmd = str(glslc)
    else:
        glslc_cmd = shutil.which("glslc")
        if not glslc_cmd:
            log("  glslc not on PATH, skipping shaders.")
            return

    for f in sources:
        src = Path(f)
        out = out_dir / (src.name + ".spv")
        stage = "vert" if src.suffix == ".vert" else "frag"
        cmd = [glslc_cmd, f"-fshader-stage={stage}", str(src), "-o", str(out)]
        run(cmd)
        log(f"  {src.name} -> {out.name}")

# ---------------------------------------------------------------
# Compile
# ---------------------------------------------------------------

def _needs_rebuild(srcp: Path, obj: Path, full_wipe: bool) -> bool:
    if full_wipe:
        return True
    if not obj.exists():
        return True
    return obj.stat().st_mtime < srcp.stat().st_mtime

def _prune_obj_dir(obj_dir: Path, keep: set):
    """Remove stale .o files whose source no longer exists."""
    for o in obj_dir.glob("*.o"):
        if o not in keep:
            try:
                o.unlink()
                log(f"  pruned stale {o.name}")
            except OSError:
                pass

def build(platform, full_wipe):
    platform = platform.strip()
    build_dir = get_platform_dir(platform)
    obj_dir   = build_dir / "obj"

    if full_wipe and build_dir.exists():
        log(f"Wiping {build_dir}...")
        shutil.rmtree(build_dir)

    build_dir.mkdir(parents=True, exist_ok=True)
    obj_dir.mkdir(parents=True, exist_ok=True)

    # Shaders (platform-aware)
    build_shaders(build_dir, platform)

    # ---- C files (Wren) ------------------------------------------------
    log(f"Compiling {len(C_FILES)} C files...")
    c_objs = []
    keep_objs = set()
    for src in C_FILES:
        srcp = Path(src)
        if not srcp.exists():
            raise RuntimeError(f"Missing C source: {srcp}")
        obj = obj_dir / (srcp.name + ".o")
        keep_objs.add(obj)
        if not _needs_rebuild(srcp, obj, full_wipe):
            c_objs.append(obj)
            continue
        cmd = [CC, "-std=c99", "-O2", "-c", str(srcp),
               "-I", str(WREN_SRC / "include"),
               "-I", str(WREN_SRC / "vm"),
               "-I", str(WREN_SRC / "optional"),
               "-o", str(obj)]
        run(cmd)
        log(f"  {srcp.name}")
        c_objs.append(obj)

    # ---- C++ files -----------------------------------------------------
    cpp_files = ENGINE_CPP + CPP_FILES
    log(f"Compiling {len(cpp_files)} C++ files...")
    cpp_objs = []
    for src in cpp_files:
        srcp = Path(src)
        if not srcp.exists():
            # Hard fail — silent skip was hiding real link errors.
            raise RuntimeError(f"Missing C++ source: {srcp}")
        obj = obj_dir / (srcp.name + ".o")
        keep_objs.add(obj)
        if not _needs_rebuild(srcp, obj, full_wipe):
            cpp_objs.append(obj)
            continue
        cmd = [CXX] + CXX_FLAGS + ["-c", str(srcp), "-o", str(obj)]
        result = subprocess.run(
            cmd, capture_output=True, text=True, errors="replace"
        )
        if result.returncode != 0:
            log(f"  FAIL: {srcp.name}")
            print(result.stdout)
            print(result.stderr)
            raise RuntimeError(f"Failed to compile {srcp.name}")
        log(f"  OK: {srcp.name}")
        cpp_objs.append(obj)

    # ---- Prune stale object files -------------------------------------
    _prune_obj_dir(obj_dir, keep_objs)

    # ---- Link ----------------------------------------------------------
    exe_name = get_exe_name(platform)
    out_exe  = build_dir / exe_name
    log(f"Linking {exe_name}...")

    all_objs = [str(o) for o in c_objs + cpp_objs]
    link_cmd = [CXX] + all_objs + ["-o", str(out_exe)] + get_libs(platform)
    result = subprocess.run(
        link_cmd, capture_output=True, text=True, errors="replace"
    )
    if result.returncode != 0:
        log("LINKER ERROR:")
        print(result.stdout)
        print(result.stderr)
        raise RuntimeError("Failed to link executable")

    log(f"build OK: {out_exe}")

    # ---- Python runtime payload ---------------------------------------
    if platform.lower() == "windows":
        # 1) Embeddable package (python312.zip, _pth, some DLLs)
        if PYTHON_EMBED_ROOT.exists():
            log("Copying Python embeddable package to build directory...")
            for item in PYTHON_EMBED_ROOT.iterdir():
                dest = build_dir / item.name
                try:
                    if item.is_dir():
                        shutil.copytree(str(item), str(dest), dirs_exist_ok=True)
                    else:
                        shutil.copy2(str(item), str(dest))
                    log(f"  Copied {item.name}")
                except Exception as e:
                    log(f"  WARNING: could not copy {item.name}: {e}")
        else:
            log(f"  WARNING: embeddable package not found at {PYTHON_EMBED_ROOT}")

        # 2) Ensure python312.dll is next to the exe (linker import lib is not the runtime DLL).
        dll_dest = build_dir / "python312.dll"
        if not dll_dest.exists():
            candidates = []
            if PYTHON_DLL and PYTHON_DLL.exists():
                candidates.append(PYTHON_DLL)
            if PYTHON_EMBED_ROOT.exists():
                candidates.append(PYTHON_EMBED_ROOT / "python312.dll")
            for c in candidates:
                if c.exists():
                    shutil.copy2(str(c), str(dll_dest))
                    log(f"  Copied {c.name} (runtime DLL)")
                    break
            else:
                log("  WARNING: python312.dll not found — exe may fail to start.")

        # 3) Stdlib (Lib/) — embed package lacks python312.zip, so we drop the stdlib in.
        if PYTHON_SYSTEM_ROOT and (PYTHON_SYSTEM_ROOT / "Lib").exists():
            lib_src = PYTHON_SYSTEM_ROOT / "Lib"
            lib_dest = build_dir / "Lib"
            try:
                shutil.copytree(str(lib_src), str(lib_dest), dirs_exist_ok=True)
                log("  Copied Lib/ (Python stdlib)")
                pth_dest = build_dir / "python312._pth"
                if pth_dest.exists():
                    pth_text = pth_dest.read_text(encoding="utf-8")
                    if "Lib" not in pth_text:
                        pth_dest.write_text("python312.zip\nLib\n.\n", encoding="utf-8")
                        log("  Updated python312._pth to include Lib/")
            except Exception as e:
                log(f"  WARNING: could not copy Lib/: {e}")

    return out_exe

# ---------------------------------------------------------------
# TUI
# ---------------------------------------------------------------

REPL = """
=================================
AbyssEngine / D.A.T.S build SYSTEM
=================================

[ 1 ] Build (Full Wipe and Rebuild)
[ 2 ] Clear build Directory
[ 3 ] Build (Incremental Build)
[ 4 ] Configuration
[ 5 ] Exit

>>> """

def main():
    while True:
        print(REPL)
        choice = input(">>> ").strip()
        print()

        if choice == "5":
            print("Goodbye.")
            break

        if choice in ("1", "3"):
            platform = input("What is your build Choice (Linux, Windows): ").strip()
            try:
                exe = build(platform, full_wipe=(choice == "1"))
                print(f"\nLaunching {exe}...")
                subprocess.run([str(exe)], cwd=str(exe.parent))
            except Exception as e:
                print(f"\nbuild FAILED: {e}")
        elif choice == "2":
            if BUILD_ROOT.exists():
                shutil.rmtree(BUILD_ROOT)
                print("build directory cleared.")
            else:
                print("build directory does not exist.")
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