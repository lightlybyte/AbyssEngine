# Dynami 20.09

*A tactical operations simulator built with Abyss Engine for a game jam.*

![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Vulkan](https://img.shields.io/badge/Vulkan-1.0-orange.svg)
![License](https://img.shields.io/badge/License-DSAL%201.0-green.svg)

> **Status:** Pre-alpha / game-jam engine. Not production-ready.
> See [TOWORKON.md](TOWORKON.md) for the current engineering backlog.

Dynami 20.09 is a tactical operations simulator powered by **Abyss Engine**.
The client runs in C++/Vulkan; the server prototype is under development.

## Technical Architecture

### Abyss Engine *(implemented)*

| Area | Status | Notes |
|------|--------|-------|
| Vulkan renderer | Implemented (Vulkan 1.0) | Nuklear overlay, swapchain recreation on resize/out-of-date |
| Windowing / input | Implemented | GLFW 3, events polled each frame |
| UI menu system | Implemented | Multi-screen Nuklear UI (main, settings, console, credits...) |
| Python embedding | Implemented | CPython 3.x via python.cpp; registered native callbacks |
| BPC GLB pipeline | Partial | Reads glTF 2.0 JSON + bin; classifies mesh primitives |
| Physics prototype | Partial | Simple rigid-body stepping exists (physics.cpp) |
| Game state machine | Implemented | GameState: MENU / PLAYING / PAUSED with transitions |
| Wren scripting VM | Vendor files present | Header/source included but not wired into main loop yet |

### Planned / not-yet-implemented

The following are **planned** and must not be confused with shipped functionality:

- Server-authoritative networking (UDP 120–240 Hz)
- Full ballistics / fluid-dynamics solver
- FPV / DJI controller integration
- UDM modding platform
- Custom .blend parser (BPC C++ replacement)
- opus-compressed audio streaming at target footprint
- Procedural terrain/foliage generation
- x86-64 assembly server tick path

## Asset Pipeline

- **Current:** Python BPC reads .glb -> extracts positions/normals/UVs -> classifies primitives -> emits JSON instructions.
- **Planned:** C++ GLB/BPC parser, hot-reload, cache, and runtime format versioning.

## Known Limitations

| Area | Limitation |
|------|-----------|
| Renderer | Vulkan 1.0 only; no validation layers in release builds; Intel HD Graphics 620 untested |
| Resizing | Swapchain recreates on resize; zero-sized frames throw rather than silently drop |
| Python | Single-threaded interpreter; GIL not acquired from worker threads |
| GLB support | Sparse accessors / external buffers / normalized integer accessors unsupported |
| Networking | None implemented |
| Physics | Basic stepping only; no contact caching, no vehicle dynamics |

## Build

See Compile.py for the build orchestrator.

```bash
# Windows
python Compile.py       # launches interactive TUI

# Linux (set vars before running)
export PYTHON_ROOT=/usr
export VULKAN_SDK=/opt/vulkan-sdk
python Compile.py
```

Required dependencies:
- Toolchain: GCC 9+ or MSVC 19.2+
- Vulkan SDK (provides glslc and vulkan-1.lib)
- Python 3.8+ headers and shared lib (libpython3.x.a / python3x.lib)
- GLFW 3 (vendored in include/GLFW/)
- GLM (vendored in include/glm/)
- Nuklear single-header (vendored in include/)

## License

DATS and Abyss Engine are distributed under the **Dynami 20.09 Source-Available License (DSAL) 1.0**.

This is **not** an OSI-approved open-source license. The source may be viewed and modified for personal or internal use. Commercial distribution, selling the binary, and monetization of the engine are prohibited. See LICENSE for full terms.
