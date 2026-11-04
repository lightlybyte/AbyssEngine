#!/usr/bin/env python3
"""
Abyss Engine / D.A.T.S build System
"""

import os
import re
import sys
import shutil
import subprocess
import glob
import time
import zipfile
from concurrent.futures import ThreadPoolExecutor, as_completed
from pathlib import Path

ROOT        = Path(__file__).parent.resolve()
SOURCE      = ROOT / "source"
INCLUDE     = ROOT / "include"
LIB_DIR     = ROOT / "lib"
WREN_SRC    = ROOT / "compiler" / "src"
BUILD_ROOT  = ROOT / "build"
SHADERS     = ROOT / "Shaders"
VULKAN_ROOT = ROOT / "vulkansdk"

NPROC = max(1, (os.cpu_count() or 4) - 1)

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

CXX = "g++"
CC  = "gcc"

# -Iinclude FIRST so <ui.h> / <engine.h> / <nuklear.h> resolve to the
# canonical copies under include/, never a shadow in source/.
CXX_FLAGS = [
    "-std=c++17", "-O2", "-pipe", "-MMD", "-MP",
    "-fdiagnostics-color=always",
    "-I", str(INCLUDE),
    "-I", str(SOURCE),
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

# Unified UI: ui.cpp replaces ui_theme.cpp + ui_menu.cpp.
ENGINE_CPP = [
    str(SOURCE / "vkrender.cpp"),
    str(SOURCE / "engine.cpp"),
    str(SOURCE / "main.cpp"),
    str(SOURCE / "python.cpp"),
    str(SOURCE / "ui.cpp"),
]

SYSTEM_PYTHON_ROOT = Path(
    r"C:\Users\lightlybyt\AppData\Local\Python\pythoncore-3.15-64"
)

PY_VER_RE = re.compile(r"^python(\d+)(\d+)\.(?:lib|dll\.a|zip|dll)$")

def _detect_py_version(root: Path):
    if not root.exists():
        return None
    for sub in (root / "libs", root / "lib", root / "Libs", root):
        if not sub.exists():
            continue
        for f in sub.iterdir():
            m = PY_VER_RE.match(f.name)
            if m:
                return m.group(1) + m.group(2)
    return None

def _probe_python_layout(root: Path):
    if not root or not root.exists():
        return None, None, None, None

    inc = None
    for cand in (root / "include", root / "Include", root / "headers"):
        if (cand / "Python.h").exists():
            inc = cand
            break
    if inc is None and (root / "Python.h").exists():
        inc = root
    if inc is None:
        return None, None, None, None

    ver = _detect_py_version(root)
    if ver is None:
        pl = inc / "patchlevel.h"
        if pl.exists():
            try:
                text = pl.read_text(encoding="utf-8", errors="replace")
                major = re.search(r"#define\s+PY_MAJOR_VERSION\s+(\d+)", text)
                minor = re.search(r"#define\s+PY_MINOR_VERSION\s+(\d+)", text)
                if major and minor:
                    ver = major.group(1) + minor.group(1)
            except OSError:
                pass
    if ver is None:
        return None, None, None, None

    lib = None
    names = (f"python{ver}.lib", f"libpython{ver}.a", f"python{ver}.dll.a")
    for cand in (root / "libs", root / "lib", root / "Libs", root):
        if not cand.exists():
            continue
        if any((cand / n).exists() for n in names):
            lib = cand
            break

    dll = None
    for cand in (root / f"python{ver}.dll", root / "bin" / f"python{ver}.dll"):
        if cand.exists():
            dll = cand
            break

    return inc, lib, dll, ver

def _find_python():
    candidates = []
    env = os.environ.get("DATS_PYTHON_ROOT")
    if env:
        candidates.append(Path(env))
    candidates.append(SYSTEM_PYTHON_ROOT)
    candidates.append(Path(r"C:\Python315"))
    candidates.append(Path(r"C:\Program Files\Python315"))

    appdata = Path.home() / "AppData" / "Local" / "Python"
    if appdata.exists():
        for d in appdata.glob("pythoncore-*-64"):
            if d not in candidates:
                candidates.append(d)

    candidates.append(LIB_DIR / "pythonembed")

    for root in candidates:
        inc, lib, dll, ver = _probe_python_layout(root)
        if inc is not None:
            return root, inc, lib, dll, ver
    return None, None, None, None, None

PYTHON_ROOT, PYTHON_INC, PYTHON_LIB, PYTHON_DLL, PYTHON_VER = _find_python()

if PYTHON_INC is None:
    print("[Compile] WARNING: Python.h not found in any known location.")
    PY_VER = "315"
else:
    PY_VER = PYTHON_VER
    CXX_FLAGS += ["-I", str(PYTHON_INC)]
    print(f"[Compile] Python headers : {PYTHON_INC}")
    print(f"[Compile] Python version : 3.{PY_VER[-2:]}")
    if PYTHON_LIB is not None:
        print(f"[Compile] Python libs    : {PYTHON_LIB}")
    else:
        print(f"[Compile] WARNING: python{PY_VER}.lib not found — link may fail.")
    if PYTHON_DLL is not None:
        print(f"[Compile] Python runtime : {PYTHON_DLL}")

PYTHON_EMBED_ROOT = LIB_DIR / "pythonembed"

def _preflight_shadow_check():
    if not SOURCE.exists() or not INCLUDE.exists():
        return
    shadows = []
    for ext in ("*.h", "*.hpp"):
        for f in SOURCE.glob(ext):
            other = INCLUDE / f.name
            if other.exists():
                shadows.append((f, other))
    if shadows:
        print("[Compile] WARNING: source/ headers shadow include/ headers:")
        for src, inc in shadows:
            print(f"[Compile]   {src}  shadows  {inc}")
        print("[Compile]   Delete the source/ copy. Include order now puts")
        print("[Compile]   include/ first, so the shadow is harmless, but")
        print("[Compile]   deleting it removes ambiguity for future headers.")
        print()

_preflight_shadow_check()

LIBS_WINDOWS = [x for x in [
    "-L", str(VULKAN_LIB),
    "-lvulkan-1",
    "-L", str(PYTHON_LIB) if PYTHON_LIB else "",
    f"-lpython{PY_VER}",
    "-L", str(LIB_DIR),
    "-lglfw3",
    "-lgdi32", "-luser32", "-lshell32", "-lole32",
    "-Wl,--as-needed",
] if x]

LIBS_LINUX = [
    "-lvulkan", "-lglfw",
    "-lpthread", "-ldl", "-lm", "-lX11",
    "-Wl,--as-needed",
]

if os.name == "nt":
    os.system("")

G_DIM    = "\x1b[38;2;90;90;90m"
G_MID    = "\x1b[38;2;140;140;140m"
G_TEXT   = "\x1b[38;2;200;200;200m"
G_BRIGHT = "\x1b[38;2;235;235;235m"
BG_DARK  = "\x1b[48;2;18;18;18m"
BG_PANEL = "\x1b[48;2;28;28;28m"
RST      = "\x1b[0m"
BOLD     = "\x1b[1m"

_ANSI_RE = re.compile(r"\x1b\[[0-9;]*m")

def _vis_len(s):
    return len(_ANSI_RE.sub("", s))

def _pad_to(s, width):
    n = _vis_len(s)
    if n >= width:
        return s
    return s + (" " * (width - n))

def _truncate_vis(s, width):
    if _vis_len(s) <= width:
        return s
    if width <= 0:
        return ""
    out, vis, i, n = [], 0, 0, len(s)
    while i < n and vis < width - 1:
        m = _ANSI_RE.match(s, i)
        if m:
            out.append(m.group(0))
            i = m.end()
            continue
        out.append(s[i])
        vis += 1
        i += 1
    return "".join(out) + RST + "…"

def _fit(s, width):
    if _vis_len(s) > width:
        return _pad_to(_truncate_vis(s, width), width)
    return _pad_to(s, width)

def _term():
    try:
        size = shutil.get_terminal_size((80, 24))
        cols, rows = size.columns, size.lines
    except Exception:
        cols, rows = 80, 24
    cols = max(60, min(cols, 120))
    rows = max(20, rows)
    return cols, rows

def _clear():
    sys.stdout.write("\x1b[2J\x1b[H")
    sys.stdout.flush()

def _hide_cursor():
    sys.stdout.write("\x1b[?25l")
    sys.stdout.flush()

def _show_cursor():
    sys.stdout.write("\x1b[?25h")
    sys.stdout.flush()

def _write_frame(lines):
    sys.stdout.write("\n".join(lines) + "\n")
    sys.stdout.flush()

def log(msg):
    print(f"  {G_MID}·{RST} {G_TEXT}{msg}{RST}", flush=True)

def run(cmd, cwd=None):
    r = subprocess.run(cmd, cwd=cwd, capture_output=True, text=True, errors="replace")
    if r.returncode != 0:
        print(r.stdout); print(r.stderr)
        raise RuntimeError(f"Command failed: {' '.join(cmd)}")
    return r

def get_libs(platform):
    return LIBS_LINUX if platform.lower() == "linux" else LIBS_WINDOWS

def get_exe_name(platform):
    return "DATS.exe" if platform.lower() == "windows" else "DATS"

def get_platform_dir(platform):
    return BUILD_ROOT / platform.capitalize()

def _read_depfile(obj):
    d = obj.with_suffix(".d")
    if not d.exists():
        return []
    try:
        text = d.read_text(encoding="utf-8", errors="replace").replace("\\\n", " ")
    except OSError:
        return []
    if ":" not in text:
        return []
    _, _, deps = text.partition(":")
    out = []
    for raw in deps.split():
        p = Path(raw.strip())
        if p.exists():
            out.append(p)
    return out

def _needs_rebuild(srcp, obj, full_wipe):
    if full_wipe or not obj.exists():
        return True
    if obj.stat().st_mtime < srcp.stat().st_mtime:
        return True
    for dep in _read_depfile(obj):
        try:
            if dep.stat().st_mtime > obj.stat().st_mtime:
                return True
        except OSError:
            return True
    return False

def _prune_obj_dir(obj_dir, keep):
    for o in obj_dir.glob("*.o"):
        if o not in keep:
            try:
                o.unlink()
                log(f"pruned stale {o.name}")
            except OSError:
                pass

def build_shaders(build_dir, platform):
    log("building shaders with glslc…")
    out_dir = build_dir / "Shaders"
    out_dir.mkdir(parents=True, exist_ok=True)
    if not SHADERS.exists():
        log("no Shaders/ folder, skipping")
        return
    sources = []
    for ext in ("*.vert", "*.frag"):
        sources += glob.glob(str(SHADERS / ext))
    if not sources:
        log("no shaders found")
        return

    if platform.lower() == "windows":
        glslc = VULKAN_BIN / "glslc.exe"
        if not glslc.exists():
            log(f"glslc.exe not found in {VULKAN_BIN}, skipping shaders")
            return
        glslc_cmd = str(glslc)
    else:
        glslc_cmd = shutil.which("glslc")
        if not glslc_cmd:
            log("glslc not on PATH, skipping shaders")
            return

    def _one(f):
        src = Path(f)
        out = out_dir / (src.name + ".spv")
        stage = "vert" if src.suffix == ".vert" else "frag"
        run([glslc_cmd, f"-fshader-stage={stage}", str(src), "-o", str(out)])
        return src.name

    with ThreadPoolExecutor(max_workers=NPROC) as pool:
        for name in pool.map(_one, sources):
            log(f"{name} -> {name}.spv")

def _compile_one(cmd, srcp, obj):
    r = subprocess.run(cmd, capture_output=True, text=True, errors="replace")
    return (srcp, r.stdout if r.returncode else None, r.stderr)

def _build_objects(label, cmd_builder, files, obj_dir, full_wipe):
    objs, keep, to_build = [], set(), []
    for src in files:
        srcp = Path(src)
        if not srcp.exists():
            raise RuntimeError(f"Missing source: {srcp}")
        obj = obj_dir / (srcp.name + ".o")
        keep.add(obj)
        objs.append(obj)
        if not _needs_rebuild(srcp, obj, full_wipe):
            continue
        to_build.append((srcp, obj, cmd_builder(srcp, obj)))

    total = len(objs)
    cached = total - len(to_build)
    log(f"{label}: {cached}/{total} cached, {len(to_build)} rebuilding × {NPROC}")
    if not to_build:
        return objs, keep

    failures = []
    done = 0
    with ThreadPoolExecutor(max_workers=NPROC) as pool:
        futures = [pool.submit(_compile_one, cmd, srcp, obj)
                   for (srcp, obj, cmd) in to_build]
        for fut in as_completed(futures):
            srcp, out, err = fut.result()
            done += 1
            if out is not None:
                failures.append((srcp, out, err))
                log(f"{label} FAIL [{done}/{len(to_build)}]: {srcp.name}")
            else:
                log(f"{label} OK   [{done}/{len(to_build)}]: {srcp.name}")

    if failures:
        for srcp, out, err in failures:
            print(f"\n{G_BRIGHT}--- {srcp.name} ---{RST}")
            print(out); print(err)
        raise RuntimeError(f"{len(failures)} file(s) failed to compile")
    return objs, keep

def build(platform, full_wipe):
    platform = platform.strip()
    build_dir = get_platform_dir(platform)
    obj_dir = build_dir / "obj"
    t0 = time.time()

    log(f"→ {platform}  ·  {'wipe' if full_wipe else 'incremental'}  ·  {NPROC} workers")

    if full_wipe and build_dir.exists():
        log(f"wiping {build_dir.name}…")
        shutil.rmtree(build_dir)
    build_dir.mkdir(parents=True, exist_ok=True)
    obj_dir.mkdir(parents=True, exist_ok=True)

    build_shaders(build_dir, platform)

    def c_cmd(srcp, obj):
        return [
            CC, "-std=c99", "-O2", "-pipe", "-c", str(srcp),
            "-I", str(WREN_SRC / "include"),
            "-I", str(WREN_SRC / "vm"),
            "-I", str(WREN_SRC / "optional"),
            "-MMD", "-MP",
            "-o", str(obj),
        ]
    c_objs, c_keep = _build_objects("C", c_cmd, C_FILES, obj_dir, full_wipe)

    def cpp_cmd(srcp, obj):
        return [CXX] + CXX_FLAGS + ["-c", str(srcp), "-o", str(obj)]
    cpp_objs, cpp_keep = _build_objects(
        "C++", cpp_cmd, ENGINE_CPP + CPP_FILES, obj_dir, full_wipe
    )

    _prune_obj_dir(obj_dir, c_keep | cpp_keep)

    exe_name = get_exe_name(platform)
    out_exe = build_dir / exe_name
    log(f"linking {exe_name}…")
    link_cmd = ([CXX] + [str(o) for o in c_objs + cpp_objs]
                + ["-o", str(out_exe)] + get_libs(platform))
    r = subprocess.run(link_cmd, capture_output=True, text=True, errors="replace")
    if r.returncode != 0:
        print(r.stdout); print(r.stderr)
        raise RuntimeError("link failed")

    log(f"build OK → {out_exe}  ({time.time() - t0:.1f}s)")

    if platform.lower() == "windows":
        _prepare_python_payload(build_dir)

    return out_exe

def _zip_name(): return f"python{PY_VER}.zip"
def _dll_name(): return f"python{PY_VER}.dll"
def _pth_name(): return f"python{PY_VER}._pth"

def _validate_embed_package(root):
    problems = []
    if not root.exists():
        return [f"embeddable package missing at {root}"]
    zip_path = root / _zip_name()
    if not zip_path.exists():
        problems.append(f"missing {zip_path}")
    else:
        try:
            with zipfile.ZipFile(zip_path) as zf:
                names = set(zf.namelist())
            if "encodings/__init__.py" not in names:
                problems.append(f"{_zip_name()} does not contain encodings/")
        except zipfile.BadZipFile:
            problems.append(f"{zip_path} is not a valid zip")
    if not (root / _dll_name()).exists():
        problems.append(f"missing {root / _dll_name()}")
    return problems

def _prepare_python_payload(build_dir):
    if not PYTHON_EMBED_ROOT.exists():
        log(f"embeddable package missing at {PYTHON_EMBED_ROOT}")
        if PYTHON_DLL and PYTHON_DLL.exists():
            shutil.copy2(str(PYTHON_DLL), str(build_dir / _dll_name()))
            log(f"copied {_dll_name()} (fallback)")
        return

    log("preparing Python embeddable payload…")
    problems = _validate_embed_package(PYTHON_EMBED_ROOT)
    if problems:
        log("embeddable package is broken:")
        for p in problems:
            log(f"  • {p}")
        log("falling back to system Python stdlib copy…")
        _fallback_copy_stdlib_from_system(build_dir)
        return

    copied = 0
    for item in PYTHON_EMBED_ROOT.iterdir():
        dest = build_dir / item.name
        try:
            if item.is_dir():
                shutil.copytree(str(item), str(dest), dirs_exist_ok=True)
            else:
                shutil.copy2(str(item), str(dest))
            copied += 1
        except Exception as e:
            log(f"  WARNING: could not copy {item.name}: {e}")
    log(f"copied {copied} item(s) from embed package (flat layout)")

    (build_dir / _pth_name()).write_text(
        f"{_zip_name()}\n.\nLibs\nimport site\n", encoding="utf-8")
    log(f"wrote {_pth_name()}")

    dll_dest = build_dir / _dll_name()
    if not dll_dest.exists():
        dll_src = PYTHON_EMBED_ROOT / _dll_name()
        if dll_src.exists():
            shutil.copy2(str(dll_src), str(dll_dest))
            log(f"copied {_dll_name()} (runtime)")
        elif PYTHON_DLL and PYTHON_DLL.exists():
            shutil.copy2(str(PYTHON_DLL), str(dll_dest))
            log(f"copied {_dll_name()} (fallback)")

    log(f"payload OK ({_zip_name()} + {_pth_name()} + {_dll_name()})")

def _fallback_copy_stdlib_from_system(build_dir):
    if not PYTHON_ROOT:
        log("  no system Python found either — cannot recover")
        return
    lib_src = PYTHON_ROOT / "Lib"
    if not lib_src.exists():
        log(f"  {lib_src} not found"); return
    lib_dest = build_dir / "Lib"
    log(f"  copying {lib_src} → {lib_dest} (this takes a moment)")
    shutil.copytree(str(lib_src), str(lib_dest), dirs_exist_ok=True)
    if PYTHON_DLL and PYTHON_DLL.exists():
        shutil.copy2(str(PYTHON_DLL), str(build_dir / _dll_name()))
        log(f"  copied {_dll_name()}")
    (build_dir / _pth_name()).write_text("Lib\n.\nimport site\n", encoding="utf-8")
    log(f"  wrote {_pth_name()} (Lib/ fallback)")
    log("  payload OK (fallback via system Lib/)")

MENU_ITEMS = [
    ("1", "Build",       "Full wipe and rebuild"),
    ("2", "Clear",       "Delete the build directory"),
    ("3", "Incremental", "Rebuild only what changed"),
    ("4", "Config",      "Not yet implemented"),
    ("5", "Exit",        "Close the build system"),
]

H, V = "─", "│"
TL, TR, BL, BR = "╭", "╮", "╰", "╯"
LT, RT = "├", "┤"

def _menu_frame(cols, platform_hint, last_action):
    inner = cols - 2
    out = []
    def sep(): return f"{G_DIM}{LT}{H * inner}{RT}{RST}"
    def top(): return f"{G_DIM}{TL}{H * inner}{TR}{RST}"
    def bottom(): return f"{G_DIM}{BL}{H * inner}{BR}{RST}"
    def row(content, bg=""):
        body = _fit(content, inner)
        return f"{G_DIM}{V}{RST}{bg}{body}{RST}{G_DIM}{V}{RST}"

    left = f" {G_BRIGHT}{BOLD}Abyss Engine  ·  D.A.T.S{RST}"
    right = f"{G_DIM}build system  ·  v1 {RST}"
    lw, rw = _vis_len(left), _vis_len(right)
    gap = inner - lw - rw
    if gap < 1:
        right = ""; rw = 0; gap = max(1, inner - lw)
    out.append(top())
    out.append(f"{G_DIM}{V}{RST}{BG_DARK}{left}{' ' * gap}{right}{RST}{G_DIM}{V}{RST}")
    out.append(sep())

    if cols >= 72:
        sub = "combined-arms mil-sim  ·  no flags, no bad guys, just humans"
        out.append(row(f" {G_DIM}{sub}", bg=BG_PANEL))
        out.append(sep())

    key_w = 4
    name_w = 14 if cols >= 70 else 12
    pad = 2
    desc_w = max(8, inner - pad - key_w - 1 - name_w - 1 - pad)

    for i, (key, name, desc) in enumerate(MENU_ITEMS):
        key_cell = _pad_to(f"{G_BRIGHT}{BOLD}[{key}]{RST}", key_w)
        name_cell = _fit(f"{G_TEXT}{name}", name_w)
        desc_cell = _truncate_vis(f"{G_DIM}{desc}", desc_w)
        line = f" {key_cell} {name_cell} {_pad_to(desc_cell, desc_w)} "
        out.append(row(line, bg=BG_PANEL))
        if i < len(MENU_ITEMS) - 1 and cols >= 68:
            out.append(
                f"{G_DIM}{V}{RST}{BG_PANEL}{' ' + H * (inner - 4) + ' '}{RST}{BG_PANEL} {RST}{G_DIM}{V}{RST}"
            )

    out.append(sep())
    left_v = f"{G_DIM}platform{RST}  {G_BRIGHT}{platform_hint}{RST}"
    mid_v = f"{G_DIM}workers{RST}  {G_BRIGHT}{NPROC}{RST}"
    right_v = f"{G_DIM}{last_action}{RST}"
    col1 = max(14, inner // 3)
    col2 = max(12, inner // 3)
    col3 = inner - col1 - col2 - 2
    if col3 < 8:
        col3 = 0
    parts = [_fit(f" {left_v}", col1), _fit(f" {mid_v}", col2)]
    if col3 > 0:
        parts.append(_fit(f" {right_v}", col3))
    out.append(row("".join(parts), bg=BG_PANEL))
    out.append(bottom())
    return out

def _draw_menu(platform_hint, last_action):
    cols, _ = _term()
    _clear()
    frame = _menu_frame(cols, platform_hint, last_action)
    frame.append("")
    frame.append(f"  {G_BRIGHT}❯{RST} {G_DIM}choice{RST} {G_DIM}[1-5]{RST}")
    _write_frame(frame)

def _read(prompt="  ❯ "):
    try:
        return input(prompt).strip()
    except EOFError:
        return "5"

def _platform_prompt(platform_hint, last_action):
    cols, _ = _term()
    _clear()
    frame = _menu_frame(cols, platform_hint, last_action)
    frame.append("")
    frame.append(f"  {G_DIM}platform?{RST}  {G_TEXT}1{RST} Windows   "
                 f"{G_DIM}·{RST}  {G_TEXT}2{RST} Linux")
    _write_frame(frame)
    raw = _read().lower()
    if raw in ("1", "w", "win", "windows"):
        return "windows"
    if raw in ("2", "l", "lin", "linux"):
        return "linux"
    return raw or "windows"

def main():
    _hide_cursor()
    platform_hint = "—"
    last_action = "ready"
    try:
        while True:
            _draw_menu(platform_hint, last_action)
            choice = _read()
            if choice == "5":
                _clear()
                print(f"\n  {G_BRIGHT}Goodbye.{RST}\n")
                break
            if choice in ("1", "3"):
                platform = _platform_prompt(platform_hint, last_action)
                platform_hint = platform.capitalize()
                mode = "full wipe" if choice == "1" else "incremental"
                last_action = f"{mode} → {platform_hint}"
                _clear()
                print(f"  {G_DIM}────────────────────────────────────────────{RST}")
                try:
                    exe = build(platform, full_wipe=(choice == "1"))
                    print(f"\n  {G_BRIGHT}Launching{RST} {exe}…\n")
                    subprocess.run([str(exe)], cwd=str(exe.parent))
                except Exception as e:
                    print(f"\n  {G_BRIGHT}✗ build failed:{RST} {e}")
                input(f"\n  {G_DIM}press enter to return…{RST}")
            elif choice == "2":
                last_action = "cleared"
                if BUILD_ROOT.exists():
                    shutil.rmtree(BUILD_ROOT)
                    print(f"\n  {G_BRIGHT}✓{RST} build directory cleared.")
                else:
                    print(f"\n  {G_DIM}· build directory does not exist.{RST}")
                time.sleep(0.8)
            elif choice == "4":
                last_action = "config (stub)"
                print(f"\n  {G_DIM}· configuration mode is not implemented yet.{RST}")
                time.sleep(0.8)
            else:
                last_action = "invalid"
                print(f"\n  {G_BRIGHT}✗{RST} invalid choice — pick 1-5.")
                time.sleep(0.6)
    finally:
        _show_cursor()

if __name__ == "__main__":
    try:
        main()
    except KeyboardInterrupt:
        _show_cursor()
        print("\n  Interrupted.")
        sys.exit(1)
    except Exception as e:
        _show_cursor()
        print(f"\n  build failed: {e}")
        sys.exit(1)