#!/usr/bin/env python3
"""
Abyss Engine / D.A.T.S build System
Root-level. Incremental, multithreaded. MinGW-compatible on Windows.

TUI: greyscale, box-drawn "window" with live status strip.
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

CXX       = "g++"
CC        = "gcc"

CXX_FLAGS = [
    "-std=c++17",
    "-O2",
    "-pipe",
    "-MMD", "-MP",
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
    if env:
        p = Path(env)
        if p.exists():
            return p
    embed = LIB_DIR / "pythonembed"
    if embed.exists() and (embed / "include").exists():
        return embed
    candidates = [Path(r"C:\Python312"), Path(r"C:\Program Files\Python312")]
    for c in candidates:
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

LIBS_WINDOWS = [
    "-L", str(VULKAN_LIB),
    "-lvulkan-1",
    "-L", str(PYTHON_LIB) if PYTHON_LIB else "",
    "-lpython312",
    "-L", str(LIB_DIR),
    "-lglfw3",
    "-lgdi32", "-luser32", "-lshell32", "-lole32",
    "-Wl,--as-needed",
]
LIBS_WINDOWS = [x for x in LIBS_WINDOWS if x]

LIBS_LINUX = [
    "-lvulkan", "-lglfw",
    "-lpthread", "-ldl", "-lm", "-lX11",
    "-Wl,--as-needed",
]

# ---------------------------------------------------------------
# Greyscale TUI primitives
# ---------------------------------------------------------------

# ANSI greyscale palette (truecolor-ish, but only greys — no chroma).
G_DIM   = "\x1b[38;2;90;90;90m"     #  ~  #5a5a5a
G_MID   = "\x1b[38;2;140;140;140m"  #  ~  #8c8c8c
G_TEXT  = "\x1b[38;2;200;200;200m"  #  ~  #c8c8c8
G_BRIGHT= "\x1b[38;2;235;235;235m"  #  ~  #ebebeb
BG_DARK = "\x1b[48;2;18;18;18m"     #  ~  #121212
BG_PANEL= "\x1b[48;2;28;28;28m"     #  ~  #1c1c1c
RST     = "\x1b[0m"
BOLD    = "\x1b[1m"
DIM     = "\x1b[2m"

# Box drawing — rounded corners look softer than harsh ┌┐└┘.
BOX_H  = "─"
BOX_V  = "│"
BOX_TL = "╭"
BOX_TR = "╮"
BOX_BL = "╰"
BOX_BR = "╯"
BOX_LT = "├"
BOX_RT = "┤"
BOX_TT = "┬"
BOX_BT = "┴"
BOX_X  = "┼"

def _term_width(default=78):
    try:
        return shutil.get_terminal_size((default, 24)).columns
    except Exception:
        return default

def _term_height(default=24):
    try:
        return shutil.get_terminal_size((default, 24)).lines
    except Exception:
        return default

def _fit(text, width):
    """Fit text to width, truncating with … if needed (counts visible chars only)."""
    # strip ANSI to count
    import re
    vis = re.sub(r"\x1b\[[0-9;]*m", "", text)
    if len(vis) <= width:
        return text + " " * (width - len(vis))
    # naive truncate on visible chars, keep ANSI prefix if we can
    return vis[: max(0, width - 1)] + "…"

def _clear():
    if os.name == "nt":
        os.system("")
    sys.stdout.write("\x1b[2J\x1b[H")
    sys.stdout.flush()

def _hide_cursor():
    sys.stdout.write("\x1b[?25l")
    sys.stdout.flush()

def _show_cursor():
    sys.stdout.write("\x1b[?25h")
    sys.stdout.flush()

def _hr(width, ch=BOX_H):
    return ch * width

def _center(text, width):
    vis = len(text)
    if vis >= width:
        return text
    pad = width - vis
    left = pad // 2
    right = pad - left
    return " " * left + text + " " * right

def _panel(title, lines, width, pad=1):
    """
    Rounded box-drawn panel with an inline title on the top border.
    Returns a list of rendered lines (no trailing newline).
    """
    inner_w = width - 2
    out = []
    # Top border with title
    if title:
        t = f" {title} "
        vis_t = len(t)
        left = (inner_w - vis_t) // 2
        right = inner_w - vis_t - left
        out.append(
            f"{G_MID}{BOX_TL}{BOX_H * left}{RST}"
            f"{G_BRIGHT}{BOLD}{t}{RST}"
            f"{G_MID}{BOX_H * right}{BOX_TR}{RST}"
        )
    else:
        out.append(f"{G_MID}{BOX_TL}{BOX_H * inner_w}{BOX_TR}{RST}")
    # Body
    for ln in lines:
        body = _fit(ln, inner_w - pad * 2) if isinstance(ln, str) else ln
        out.append(
            f"{G_MID}{BOX_V}{RST}"
            f"{' ' * pad}{body}{' ' * pad}"
            f"{G_MID}{BOX_V}{RST}"
        )
    # Bottom
    out.append(f"{G_MID}{BOX_BL}{BOX_H * inner_w}{BOX_BR}{RST}")
    return out

# ---------------------------------------------------------------
# Helpers
# ---------------------------------------------------------------

def log(msg):
    # Route through a status line instead of raw prints so the TUI stays framed.
    _status(msg)

_STATUS_LAST = ""
def _status(msg, kind="info"):
    global _STATUS_LAST
    _STATUS_LAST = msg
    if kind == "ok":
        prefix = f"{G_BRIGHT}✓{RST}"
    elif kind == "fail":
        prefix = f"{G_BRIGHT}✗{RST}"
    else:
        prefix = f"{G_DIM}·{RST}"
    # Print on its own line, dim — keeps scrollback readable if terminal
    # doesn't support the framed TUI (CI, dumb terminals).
    print(f"  {prefix} {G_TEXT}{msg}{RST}", flush=True)

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
    return LIBS_LINUX if platform.lower() == "linux" else LIBS_WINDOWS

def get_exe_name(platform):
    return "DATS.exe" if platform.lower() == "windows" else "DATS"

def get_platform_dir(platform):
    return BUILD_ROOT / platform.capitalize()

# ---------------------------------------------------------------
# Dependency file parsing
# ---------------------------------------------------------------

def _read_depfile(obj: Path) -> list[Path]:
    d = obj.with_suffix(".d")
    if not d.exists():
        return []
    try:
        text = d.read_text(encoding="utf-8", errors="replace")
    except OSError:
        return []
    text = text.replace("\\\n", " ")
    if ":" not in text:
        return []
    _, _, deps = text.partition(":")
    paths = []
    for raw in deps.split():
        p = Path(raw.strip())
        if p.exists():
            paths.append(p)
    return paths

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

    def _compile_one(f):
        src = Path(f)
        out = out_dir / (src.name + ".spv")
        stage = "vert" if src.suffix == ".vert" else "frag"
        run([glslc_cmd, f"-fshader-stage={stage}", str(src), "-o", str(out)])
        return src.name

    with ThreadPoolExecutor(max_workers=NPROC) as pool:
        for name in pool.map(_compile_one, sources):
            log(f"{name} -> {name}.spv")

# ---------------------------------------------------------------
# Compile
# ---------------------------------------------------------------

def _compile_one(cmd, srcp: Path, obj: Path):
    r = subprocess.run(cmd, capture_output=True, text=True, errors="replace")
    if r.returncode != 0:
        return (srcp, r.stdout, r.stderr)
    return (srcp, None, None)

def _build_objects(label, compiler_cmd_builder, files, obj_dir, full_wipe):
    objs: list[Path] = []
    keep: set[Path] = set()
    to_build: list[tuple[Path, Path, list[str]]] = []

    for src in files:
        srcp = Path(src)
        if not srcp.exists():
            raise RuntimeError(f"Missing source: {srcp}")
        obj = obj_dir / (srcp.name + ".o")
        keep.add(obj)
        objs.append(obj)
        if not _needs_rebuild(srcp, obj, full_wipe):
            continue
        to_build.append((srcp, obj, compiler_cmd_builder(srcp, obj)))

    total = len(objs)
    cached = total - len(to_build)
    log(f"{label}: {cached}/{total} cached, {len(to_build)} rebuilding × {NPROC}")

    if not to_build:
        return objs, keep

    failures: list[tuple[Path, str, str]] = []
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
            print(out)
            print(err)
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

    all_objs = [str(o) for o in c_objs + cpp_objs]
    link_cmd = [CXX] + all_objs + ["-o", str(out_exe)] + get_libs(platform)
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
        if embed_link.is_symlink() or embed_link.exists():
            pass
        else:
            if os.name == "nt":
                r = subprocess.run(
                    ["cmd", "/c", "mklink", "/J", str(embed_link), str(PYTHON_EMBED_ROOT)],
                    capture_output=True, text=True
                )
                if r.returncode == 0:
                    log("linked pythonembed/ (junction)")
                else:
                    raise OSError("mklink /J failed")
            else:
                os.symlink(str(PYTHON_EMBED_ROOT), str(embed_link))
                log("linked pythonembed/ (symlink)")
    except Exception:
        embed_link.mkdir(exist_ok=True)
        minimal = ["python312.zip", "python312._pth", "python312.dll"]
        for name in minimal:
            src = PYTHON_EMBED_ROOT / name
            if src.exists():
                shutil.copy2(str(src), str(embed_link / name))
                log(f"copied {name}")
        libs_src = PYTHON_EMBED_ROOT / "Libs"
        if not libs_src.exists():
            libs_src = PYTHON_EMBED_ROOT / "Lib"
        if libs_src.exists():
            shutil.copytree(str(libs_src), str(embed_link / "Libs"), dirs_exist_ok=True)
            log("copied Libs/ (from embed package)")
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
# TUI — greyscale framed window
# ---------------------------------------------------------------

MENU_ITEMS = [
    ("1", "Build", "Full wipe and rebuild"),
    ("2", "Clear", "Delete the build directory"),
    ("3", "Incremental", "Rebuild only what changed"),
    ("4", "Config", "Not yet implemented"),
    ("5", "Exit", "Close the build system"),
]

def _render_menu(platform_hint: str = "—", last_action: str = "ready"):
    """
    Render the main framed menu. Returns the line count so we can clear
    on re-entry.
    """
    W = min(_term_width(), 84)
    inner = W - 2

    lines: list[str] = []

    # ── header bar ─────────────────────────────────────────────
    title   = "Abyss Engine  ·  D.A.T.S"
    version = "build system  ·  v1"
    pad     = inner - len(title) - len(version)
    if pad < 1:
        pad = 1
    lines.append(
        f"{G_MID}{BOX_TL}{BOX_H * inner}{BOX_TR}{RST}"
    )
    lines.append(
        f"{G_MID}{BOX_V}{RST}"
        f"{BG_DARK}{G_BRIGHT}{BOLD} {title}{RST}"
        f"{BG_DARK}{' ' * pad}{RST}"
        f"{BG_DARK}{G_DIM}{version} {RST}"
        f"{G_MID}{BOX_V}{RST}"
    )
    lines.append(
        f"{G_MID}{BOX_LT}{BOX_H * inner}{BOX_RT}{RST}"
    )

    # ── subtitle strip ────────────────────────────────────────
    sub = "combined-arms mil-sim  ·  no flags, no bad guys, just humans"
    lines.append(
        f"{G_MID}{BOX_V}{RST} {G_DIM}{_fit(sub, inner - 2)}{RST} {G_MID}{BOX_V}{RST}"
    )
    lines.append(
        f"{G_MID}{BOX_LT}{BOX_H * inner}{BOX_RT}{RST}"
    )

    # ── menu rows ─────────────────────────────────────────────
    key_col = 3
    name_col = 14
    for key, name, desc in MENU_ITEMS:
        key_cell  = f"{G_BRIGHT}{BOLD}[{key}]{RST}"
        # pad key cell (visible width = 3)
        name_cell = _fit(f"{G_TEXT}{name}", name_col)
        desc_cell = f"{G_DIM}{desc}{RST}"
        row = (
            f" {key_cell}  {name_cell}  {desc_cell}"
        )
        lines.append(
            f"{G_MID}{BOX_V}{RST}{BG_PANEL}{_fit(row, inner - 2)}{RST} {G_MID}{BOX_V}{RST}"
        )
        # thin divider between rows
        lines.append(
            f"{G_MID}{BOX_V}{RST}{BG_PANEL}{G_DIM}{' ' + BOX_H * (inner - 4) + ' '}{RST}{BG_PANEL} {RST}{G_MID}{BOX_V}{RST}"
        )
    # drop the last divider for a cleaner look
    lines.pop()

    # ── footer / status ───────────────────────────────────────
    lines.append(
        f"{G_MID}{BOX_LT}{BOX_H * inner}{BOX_RT}{RST}"
    )
    status_left = f"{G_DIM}platform{RST}  {G_BRIGHT}{platform_hint}{RST}"
    status_mid  = f"{G_DIM}workers{RST}  {G_BRIGHT}{NPROC}{RST}"
    status_right= f"{G_DIM}{last_action}{RST}"
    # simple 3-col layout
    col_w = inner // 3
    row = (
        f" {_fit(status_left,  col_w - 2)}"
        f" {_fit(status_mid,   col_w - 2)}"
        f" {_fit(status_right, inner - 2 * col_w)}"
    )
    lines.append(
        f"{G_MID}{BOX_V}{RST}{BG_PANEL}{_fit(row, inner - 2)}{RST}{BG_PANEL} {RST}{G_MID}{BOX_V}{RST}"
    )
    lines.append(
        f"{G_MID}{BOX_BL}{BOX_H * inner}{BOX_BR}{RST}"
    )

    # ── prompt ────────────────────────────────────────────────
    lines.append("")
    lines.append(f"  {G_BRIGHT}❯{RST} {G_TEXT}choice{RST} {G_DIM}[1-5]{RST}")

    print("\n".join(lines), flush=True)

def _prompt():
    """Inline prompt below the menu."""
    try:
        return input(f"  {G_BRIGHT}❯{RST} ").strip()
    except EOFError:
        return "5"

def _preflight_platform():
    """Tiny prompt inside the frame for platform selection."""
    print(flush=True)
    print(f"  {G_DIM}platform?{RST}  {G_TEXT}1{RST} Windows  {G_DIM}·{RST}  {G_TEXT}2{RST} Linux", flush=True)
    try:
        raw = input(f"  {G_BRIGHT}❯{RST} ").strip().lower()
    except EOFError:
        return "windows"
    if raw in ("1", "w", "win", "windows"):
        return "windows"
    if raw in ("2", "l", "lin", "linux"):
        return "linux"
    return raw or "windows"

def main():
    _clear()
    _hide_cursor()

    platform_hint = "—"
    last_action   = "ready"

    try:
        while True:
            _clear()
            _render_menu(platform_hint, last_action)
            choice = _prompt()

            if choice == "5":
                _clear()
                print(f"\n  {G_BRIGHT}Goodbye.{RST}\n")
                break

            if choice in ("1", "3"):
                _clear()
                platform = _preflight_platform()
                platform_hint = platform.capitalize()
                mode = "full wipe" if choice == "1" else "incremental"
                last_action = f"{mode} → {platform_hint}"
                _clear()
                _render_menu(platform_hint, last_action)
                print(f"\n  {G_DIM}────────────────────────────────────────────{RST}")
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
