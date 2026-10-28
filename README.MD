# DATS (Dynamic Arms Training Simulator) (Made in my Abyss Engine! For a friend game jam!)
## Made with a ~$0.79 budget (unspent)

![C++](https://img.shields.io/badge/C%2B%2B-17-blue.svg)
![C](https://img.shields.io/badge/C-11-lightgrey.svg)
![Vulkan](https://img.shields.io/badge/Vulkan-1.3-orange.svg)
![AbyssEngine](https://img.shields.io/badge/AbyssEngine-vQ1.2026.1%20%2F%20Luminance-black.svg)
![License](https://img.shields.io/badge/License-DSAL%201.0-green.svg)

DATS is a tactical military simulation powered by **Abyss Engine**. 

The client is entirely hand-coded, utilizing a pure C/C++ architecture with Vulkan for rendering and AbyssEngine for specific high-performance subsystems. The server is written from scratch in pure C and x86-64 Assembly, operating at 120Hz to 240Hz.

## Technical Architecture

### Abyss Engine
Abyss Engine is a custom, purpose-built game engine designed for extreme efficiency and low-end hardware compatibility.
- **Client:** Hand-coded C/C++ with Vulkan rendering. Utilizes AbyssEngine for data-oriented systems where appropriate.
- **Server:** Pure C and Assembly. No garbage collection. Cycle-level determinism.
- **Netcode:** Authoritative 120Hz to 240Hz tick rate. Custom UDP protocol with packet obfuscation.
- **Physics:** Unified solver for vehicles and biological entities. Simulates structural integrity, ballistics, and fluid dynamics.

### Asset Pipeline
- **Procedural Generation:** Bulk assets (terrain, foliage, clutter) are generated at runtime to minimize binary size.
- **Custom Parsers:** Features a custom C/C++ parser for `.blend` files (BPC/MDP), eliminating the need for external export tools.
- **Optimized Formats:** Uses runtime generated assets with some stored on disk, this balances binary size and QOL and speed of load.
- **Audio:** Opus compression for all audio. The system compresses 500GB of source samples into a 10MB runtime footprint. (I mean, i hand picked em...)

### Performance Targets
- **Storage:** 1GB Maximum (Friends Game Jam Reqs)
- **RAM:** 4GB minimum
- **GPU:** Intel HD 620 (128MB VRAM)
- **OS:** Windows 8 / Linux Kernel 4.0+

## Features

- **Custom Server Architecture:** Written in C and C++. No third-party netcode frameworks.
- **Authoritative Physics:** Server validates all movement, ballistics, and collision. Impossible physics are rejected at the packet level.
- **Hardcore Simulation:** No health bars. No crosshairs. Players must physically check their bodies for trauma, bleeding, and fractures.
- **Modding Support:** Custom in-game modding platform (UDM) utilizing a custom scripting language.
- **User-Hosted Infrastructure:** LAN and embedded UPnP support for community servers.
- **FPV Drone Support:** Native integration for DJI FPV controllers.

## License

This project is licensed under the **DATS Source-Available License (DSAL) 1.0**. 
The source code is open for viewing, studying, and modding. Commercial distribution, selling the binary, or monetizing the engine is strictly prohibited. See the [LICENSE](LICENSE) file for details.

## Status

Alpha development. The engine is currently being created, ported, and optimized. 

---

## Support the Developer

DATS and Abyss Engine are developed entirely independently with a $0.79 budget. There is no publisher, no investors, and no monetization in the game itself. 

If you wish to support the ongoing development, you can send donations via the following cryptocurrency networks:

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

We apreciate any funding of any size
This is not for revenue, this is for the people.