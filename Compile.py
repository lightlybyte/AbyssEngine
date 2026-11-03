#!/usr/bin/env python3
"""
Abyss Engine / D.A.T.S build System
Root-level. Incremental, multithreaded. MinGW-compatible on Windows.

TUI: greyscale, box-drawn window, adaptive to terminal width.
"""

import os
import re
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

# ---------------------------------------------------------------
# C++ Config
# ---------------------------------------------------------------

CXX = "g++"
CC  = "gcc"

CXX_FLAGS = [
    "-std=c++17", "-O2", "-pipe", "-MMD", "-MP",
    "-fdiagnostics-color=always",
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

ENGINE_CPP = [
    str(SOURCE / "vkrender.cpp"),
    str(SOURCE / "engine.cpp"),
    str(SOURCE / "main.cpp"),
    str(SOURCE / "python.cpp"),
]

def _find_python_root():
    env = os.environ.get("DATS_PYTHON_ROOT")
    if env and Path(env).exists():
        return Path(env)
    embed = LIB_DIR / "pythonembed"
    if embed.exists() and (embed / "include").exists():
        return embed
    for c in (Path(r"C:\Python312"), Path(r"C:\Program Files\Python312")):
        if c.exists():
            return c
    return None

PYTHON_SYSTEM_ROOT = _find_python_root()
if PYTHON_SYSTEM_ROOT is not None:
    PYTHON_INC = PYTHON_SYSTEM_ROOT / "Include"
    if not PYTHON_INC.exists():
        PYTHON_INC = PYTHON_SYSTEM_ROOT / "include"
    PYTHON_LIB = PYTHON_SYSTEM_ROOT / "libs"
    if not PYTHON_LIB.exists():
        PYTHON_LIB = PYTHON_SYSTEM_ROOT
    PYTHON_DLL = PYTHON_SYSTEM_ROOT / "python312.dll"
    if PYTHON_INC.exists():
        CXX_FLAGS += ["-I", str(PYTHON_INC)]
else:
    PYTHON_INC = PYTHON_LIB = PYTHON_DLL = None

PYTHON_EMBED_ROOT = LIB_DIR / "pythonembed"

LIBS_WINDOWS = [x for x in [
    "-L", str(VULKAN_LIB),
    "-lvulkan-1",
    "-L", str(PYTHON_LIB) if PYTHON_LIB else "",
    "-lpython312",
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

# ---------------------------------------------------------------
# ANSI / greyscale palette
# ---------------------------------------------------------------

if os.name == "nt":
    os.system("")  # enable VT processing

G_DIM    = "\x1b[38;2;90;90;90m"
G_MID    = "\x1b[38;2;140;140;140m"
G_TEXT   = "\x1b[38;2;200;200;200m"
G_BRIGHT = "\x1b[38;2;235;235;235m"
BG_DARK  = "\x1b[48;2;18;18;18m"
BG_PANEL = "\x1b[48;2;28;28;28m"
RST      = "\x1b[0m"
BOLD     = "\x1b[1m"

_ANSI_RE = re.compile(r"\x1b\[[0-9;]*m")

def _vis_len(s: str) -> int:
    return len(_ANSI_RE.sub("", s))

def _pad_to(s: str, width: int) -> str:
    n = _vis_len(s)
    if n >= width:
        return s
    return s + (" " * (width - n))

def _truncate_vis(s: str, width: int) -> str:
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

def _fit(s: str, width: int) -> str:
    if _vis_len(s) > width:
        return _pad_to(_truncate_vis(s, width), width)
    return _pad_to(s, width)

# ---------------------------------------------------------------
# Terminal helpers
# ---------------------------------------------------------------

def _term() -> tuple[int, int]:
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

def _write_frame(lines: list[str]):
    sys.stdout.write("\n".join(lines) + "\n")
    sys.stdout.flush()

# ---------------------------------------------------------------
# Build helpers
# ---------------------------------------------------------------

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

# ---------------------------------------------------------------
# Dependency files
# ---------------------------------------------------------------

def _read_depfile(obj: Path) -> list[Path]:
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

def _needs_rebuild(srcp: Path, obj: Path, full_wipe: bool) -> bool:
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

def _prune_obj_dir(obj_dir: Path, keep: set):
    for o in obj_dir.glob("*.o"):
        if o not in keep:
            try:
                o.unlink()
                log(f"pruned stale {o.name}")
            except OSError:
                pass

# ---------------------------------------------------------------
# Shaders
# ---------------------------------------------------------------

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

# ---------------------------------------------------------------
# Compile
# ---------------------------------------------------------------

def _compile_one(cmd, srcp: Path, obj: Path):
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
    obj_dir   = build_dir / "obj"
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
    out_exe  = build_dir / exe_name
    log(f"linking {exe_name}…")
    link_cmd = [CXX] + [str(o) for o in c_objs + cpp_objs] + ["-o", str(out_exe)] + get_libs(platform)
    r = subprocess.run(link_cmd, capture_output=True, text=True, errors="replace")
    if r.returncode != 0:
        print(r.stdout); print(r.stderr)
        raise RuntimeError("link failed")

    log(f"build OK → {out_exe}  ({time.time() - t0:.1f}s)")

    if platform.lower() == "windows":
        _prepare_python_payload(build_dir)

    return out_exe

# ---------------------------------------------------------------
# Python payload
# ---------------------------------------------------------------

def _prepare_python_payload(build_dir: Path):
    if not PYTHON_EMBED_ROOT.exists():
        log(f"embeddable package missing at {PYTHON_EMBED_ROOT}")
        if PYTHON_DLL and PYTHON_DLL.exists():
            shutil.copy2(str(PYTHON_DLL), str(build_dir / "python312.dll"))
            log("copied python312.dll (fallback)")
        return

    log("preparing Python embeddable payload…")
    embed_link = build_dir / "pythonembed"
    try:
        if not embed_link.exists():
            if os.name == "nt":
                r = subprocess.run(
                    ["cmd", "/c", "mklink", "/J", str(embed_link), str(PYTHON_EMBED_ROOT)],
                    capture_output=True, text=True)
                if r.returncode != 0:
                    raise OSError("mklink /J failed")
                log("linked pythonembed/ (junction)")
            else:
                os.symlink(str(PYTHON_EMBED_ROOT), str(embed_link))
                log("linked pythonembed/ (symlink)")
    except Exception:
        embed_link.mkdir(exist_ok=True)
        for name in ("python312.zip", "python312._pth", "python312.dll"):
            src = PYTHON_EMBED_ROOT / name
            if src.exists():
                shutil.copy2(str(src), str(embed_link / name))
                log(f"copied {name}")
        libs_src = PYTHON_EMBED_ROOT / "Libs"
        if not libs_src.exists():
            libs_src = PYTHON_EMBED_ROOT / "Lib"
        if libs_src.exists():
            shutil.copytree(str(libs_src), str(embed_link / "Libs"), dirs_exist_ok=True)
            log("copied Libs/")
        sp = PYTHON_EMBED_ROOT / "site-packages"
        if sp.exists():
            shutil.copytree(str(sp), str(embed_link / "site-packages"), dirs_exist_ok=True)
            log("copied site-packages/")

    dll_dest = build_dir / "python312.dll"
    if not dll_dest.exists():
        dll_src = PYTHON_EMBED_ROOT / "python312.dll"
        if dll_src.exists():
            shutil.copy2(str(dll_src), str(dll_dest))
            log("copied python312.dll (runtime)")
        elif PYTHON_DLL and PYTHON_DLL.exists():
            shutil.copy2(str(PYTHON_DLL), str(dll_dest))
            log("copied python312.dll (fallback)")

    pth = build_dir / "python312._pth"
    if not pth.exists() and (PYTHON_EMBED_ROOT / "python312._pth").exists():
        pth.write_text("python312.zip\n.\nLibs\nimport site\n", encoding="utf-8")
        log("wrote python312._pth")

# ---------------------------------------------------------------
# TUI — adaptive, greyscale, box-drawn
# ---------------------------------------------------------------

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

def _menu_frame(cols: int, platform_hint: str, last_action: str) -> list[str]:
    inner = cols - 2
    out: list[str] = []

    def sep() -> str:
        return f"{G_DIM}{LT}{H * inner}{RT}{RST}"

    def top() -> str:
        return f"{G_DIM}{TL}{H * inner}{TR}{RST}"

    def bottom() -> str:
        return f"{G_DIM}{BL}{H * inner}{BR}{RST}"

    def row(content: str, bg: str = "") -> str:
        body = _fit(content, inner)
        return f"{G_DIM}{V}{RST}{bg}{body}{RST}{G_DIM}{V}{RST}"

    # ── titlebar ──────────────────────────────────────────────
    left  = f" {G_BRIGHT}{BOLD}Abyss Engine  ·  D.A.T.S{RST}"
    right = f"{G_DIM}build system  ·  v1 {RST}"
    lw, rw = _vis_len(left), _vis_len(right)
    gap = inner - lw - rw
    if gap < 1:
        right = ""
        rw = 0
        gap = max(1, inner - lw)
    out.append(top())
    out.append(f"{G_DIM}{V}{RST}{BG_DARK}{left}{' ' * gap}{right}{RST}{G_DIM}{V}{RST}")
    out.append(sep())

    # ── subtitle (only if wide enough) ────────────────────────
    if cols >= 72:
        sub = "combined-arms mil-sim  ·  no flags, no bad guys, just humans"
        out.append(row(f" {G_DIM}{sub}", bg=BG_PANEL))
        out.append(sep())

    # ── menu rows ─────────────────────────────────────────────
    key_w  = 4
    name_w = 14 if cols >= 70 else 12
    pad    = 2
    desc_w = max(8, inner - pad - key_w - 1 - name_w - 1 - pad)

    for i, (key, name, desc) in enumerate(MENU_ITEMS):
        key_cell  = _pad_to(f"{G_BRIGHT}{BOLD}[{key}]{RST}", key_w)
        name_cell = _fit(f"{G_TEXT}{name}", name_w)
        desc_cell = _truncate_vis(f"{G_DIM}{desc}", desc_w)
        line = f" {key_cell} {name_cell} {_pad_to(desc_cell, desc_w)} "
        out.append(row(line, bg=BG_PANEL))
        if i < len(MENU_ITEMS) - 1 and cols >= 68:
            out.append(
                f"{G_DIM}{V}{RST}{BG_PANEL}{' ' + H * (inner - 4) + ' '}{RST}{BG_PANEL} {RST}{G_DIM}{V}{RST}"
            )

    out.append(sep())

    # ── status strip ──────────────────────────────────────────
    left_v  = f"{G_DIM}platform{RST}  {G_BRIGHT}{platform_hint}{RST}"
    mid_v   = f"{G_DIM}workers{RST}  {G_BRIGHT}{NPROC}{RST}"
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

def _draw_menu(platform_hint: str, last_action: str):
    cols, _ = _term()
    _clear()
    frame = _menu_frame(cols, platform_hint, last_action)
    frame.append("")
    frame.append(f"  {G_BRIGHT}❯{RST} {G_DIM}choice{RST} {G_DIM}[1-5]{RST}")
    _write_frame(frame)

def _read(prompt: str = "  ❯ ") -> str:
    try:
        return input(prompt).strip()
    except EOFError:
        return "5"

def _platform_prompt(platform_hint: str, last_action: str) -> str:
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
    last_action   = "ready"

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
