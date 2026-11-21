# Dynami 20.09

*A realistic, physics-driven simulation built with Abyss Engine.*

![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![Vulkan](https://img.shields.io/badge/Graphics-Vulkan-orange.svg)
![Platform](https://img.shields.io/badge/Platform-Windows%20%7C%20Linux-lightgrey.svg)
![License](https://img.shields.io/badge/License-DSAL-green.svg)
![Status](https://img.shields.io/badge/Status-Pre--Alpha-red.svg)

> **Status:** Pre-alpha. Under active development. Not production-ready.

See [TOWORKON.md](TOWORKON.md) for the engineering backlog.

---

## About

**Dynami 20.09** is a physics-driven simulation set in a fictional city, developed using the custom **Abyss Engine**.

Dynami prioritises systemic interactions, physical consequences, environmental simulation and believable character behaviour.

Rather than relying on conventional arcade-style approximations, Dynami aims to simulate the underlying systems that govern its world.

Dynami is part of a larger series. Each instalment can feature its own setting, characters, events and continuity.

### Design principles

- Physical consistency over arcade-style shortcuts.
- Persistent consequences for player actions.
- Believable environmental and character behaviour.
- Detailed vehicle and rigid-body physics.
- Reusable character assets and deformable clothing.
- Performance-conscious engineering.
- Cross-platform support.

---

## Technical Architecture

### Abyss Engine

Abyss Engine is the custom technology powering Dynami.

| Component | Status | Description |
|---|---|---|
| Vulkan renderer | Implemented | Hardware-accelerated rendering |
| Windows support | Implemented | Native Windows build |
| Linux support | Implemented | Native Linux build |
| Cross-compilation | Implemented | Build targets through the Python build system |
| Windowing and input | Implemented | GLFW-based input and window management |
| Physics engine | Implemented | Custom rigid-body solver |
| Collision response | Implemented | Physical collision handling |
| Constraint resolution | Implemented | Rigid-body constraint handling |
| Character system | In development | Reusable character models and clothing |
| Cloth simulation | In development | Deformable clothing |
| Weather simulation | In development | Environmental conditions and their effects |
| Phone interfaces | Planned | HTML, CSS and JavaScript-based device interfaces |

Implementation status refers to the current development branch and may change.

### Physics

Dynami uses a custom physics architecture.

Physics processing is performed across CPU and GPU workloads, with the CPU remaining primarily authoritative.

The engine includes:

- Rigid-body simulation.
- Collision detection and response.
- Constraint resolution.
- Physical interactions between simulated objects.

The goal is to make physical interactions produce consequences based on the properties of the objects involved, rather than relying on predetermined arcade-style outcomes.

### Character System

Dynami uses reusable human character models.

The character production workflow uses:

- Reallusion Character Creator 5 for base human models.
- Blender for clothing creation and asset preparation.
- Custom runtime systems for character behaviour and simulation.

Clothing is intended to use deformable mesh simulation rather than being limited to conventional skeletal animation.

### Phone System

Dynami features physically usable in-game mobile phones.

Their interfaces are built using:

- HTML
- CSS
- JavaScript

Individual devices can have different interfaces, layouts and hardware capabilities.

The phone system is designed around the technological period represented by each instalment.

---

## Build System

Dynami uses a Python-based TUI build orchestrator.

The build system supports Windows and Linux targets, including cross-compilation.

### Build

```bash
# Windows / Linux
python Compile.py
```

The script launches the interactive build interface.

### Dependencies

The build environment requires the appropriate:

- C++ compiler and toolchain.
- Vulkan development libraries and SDK components.
- Platform-specific dependencies.
- Python runtime.

Consult the build configuration for the exact requirements of each target.

---

## Asset Pipeline

Dynami uses a custom asset pipeline for preparing and importing game assets.

Character assets are created externally and integrated into the engine.

The pipeline is intended to support efficient asset processing, runtime compatibility and reproducible builds.

---

## Performance and Distribution

Dynami is developed with low-end hardware compatibility as an engineering consideration.

The project aims to minimise unnecessary runtime overhead and keep the distribution footprint small.

Current development builds are approximately **27 MB**, including approximately **13 MB of sound assets**. This figure may change as development progresses.

---

## Planned Development

Features and improvements that remain subject to implementation and testing include:

- Further environmental simulation.
- Expanded NPC behaviour and relationships.
- Additional vehicle systems.
- Advanced character interactions.
- Fully integrated mobile-device functionality.
- Additional asset pipeline optimisations.
- Further physics and rendering improvements.

See [TOWORKON.md](TOWORKON.md) for current priorities.

---

## Known Limitations

Dynami is pre-alpha software.

Systems may be incomplete, experimental or subject to substantial changes.

Performance, stability and compatibility are not guaranteed across all hardware configurations.

---

## License

Dynami and Abyss Engine are distributed under the **Dynami Source-Available License (DSAL)**.

This is **not an OSI-approved open-source licence**.

The licence permits certain forms of personal and non-commercial use, subject to its terms. Commercial distribution, monetisation and other restricted activities require appropriate authorisation.

See [LICENSE](LICENSE) for the complete terms.

---

## Disclaimer

Dynami is an independent project and is not affiliated with, endorsed by or associated with any real-world organisations, manufacturers or third-party intellectual property holders unless explicitly stated.

All third-party components remain subject to their respective licences.
