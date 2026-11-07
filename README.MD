# DATS — Dynamic Arms Training Simulator

*A tactical military simulation built with Abyss Engine for a friend’s game jam.*

![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![C](https://img.shields.io/badge/C-11-lightgrey.svg)
![Vulkan](https://img.shields.io/badge/Vulkan-1.3-orange.svg)
![AbyssEngine](https://img.shields.io/badge/AbyssEngine-vQ1.2026.1%20%2F%20Luminance-black.svg)
![License](https://img.shields.io/badge/License-DSAL%201.0-green.svg)

> Developed independently on a budget of approximately **$0.79**, which remains unspent.

DATS is a tactical military simulation powered by **Abyss Engine**. The client is implemented in C and C++, with Vulkan used for rendering and Abyss Engine providing selected performance-critical systems. The server is written in C and x86-64 Assembly, with a target tick rate of 120–240 Hz.

## Technical Architecture

### Abyss Engine

Abyss Engine is a custom game engine designed for performance and compatibility with low-end hardware.

- **Client:** C/C++ implementation with Vulkan rendering and data-oriented systems.
- **Server:** C and Assembly, designed without garbage collection.
- **Networking:** Authoritative 120–240 Hz server tick rate and a custom UDP protocol with packet obfuscation.
- **Physics:** Unified solver for vehicles and biological entities, including structural integrity, ballistics, and fluid dynamics.

### Asset Pipeline

- **Procedural generation:** Terrain, foliage, and environmental clutter are generated at runtime to reduce binary size.
- **Custom parsers:** A C/C++ parser for `.blend` files (BPC/MDP) is intended to remove reliance on external export tools.
- **Asset formats:** Runtime-generated and disk-based assets are used to balance storage requirements, loading time, and usability.
- **Audio:** Opus compression is used for audio. The project targets a 10 MB runtime footprint for a hand-selected library of source samples.

### Performance Targets

| Component | Target |
|---|---|
| Storage | 5 GB maximum (game jam requirement) |
| RAM | 4 GB minimum |
| GPU | Intel HD Graphics 620 (128 MB VRAM) |
| Operating system | Windows 8 / Linux kernel 4.0+ |

## Features

- **Custom server architecture:** C/C++ implementation without third-party networking frameworks.
- **Server-authoritative simulation:** Movement, ballistics, and collision are validated by the server.
- **Hardcore medical simulation:** No health bars or crosshairs. Players must inspect their characters for trauma, bleeding, and fractures.
- **Modding support:** UDM, a custom in-game modding platform with its own scripting language.
- **Community hosting:** LAN support and embedded UPnP functionality.
- **FPV drone support:** Native integration with DJI FPV controllers.

## License

DATS is distributed under the **DATS Source-Available License (DSAL) 1.0**.

The source code may be viewed, studied, and modified for modding. Commercial distribution, selling the binary, and monetizing the engine are prohibited. See [LICENSE](LICENSE) for the full terms.

## Development Status

**Alpha.** Abyss Engine is under active development, including creation, porting, and optimization.

## References and Inspirations

DATS was inspired by the gap between the hardware requirements of AAA games and the cost of the systems needed to run them. The project explores ways to reduce hardware requirements and make demanding games more accessible on lower-end systems.

## Support Development

DATS and Abyss Engine are independently developed, with no publisher, investors, or in-game monetization. The project was started with a budget of approximately **$0.79**.

Donations can be sent through the following cryptocurrency networks:

**Bitcoin (BTC)**
```text
bc1q7l3cwycx5v3aprt5rksccr9zpgzgp33yu9v9pt
```

**Ethereum (ETH)**
```text
0x86E65e135deb4EA17bA161f79021a629877ECb20
```

**Litecoin (LTC)**
```text
ltc1qy9h4ppp0py32597dx2nrqpm6zklg6934ypvjvz
```

**Solana (SOL)**
```text
HfANSA58L9bWwHcRpDfVRUpQia5LDj4oZbauhiL6noea
```

Donations of any amount are appreciated.
