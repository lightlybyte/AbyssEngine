DATS — Dynamic Arms Training Simulator
README

A combined-arms military simulation built on Abyss Engine. No flags. No bad guys. Just humans. And the Geiger meter rests.
ENGINE
Abyss Engine

Custom C++17 engine. Vulkan renderer. No Unity. No Unreal. No Godot. No CryEngine. Built from scratch.
Core Systems

AbyssEngine.dll — core engine (vkrender.cpp, third_party_impl.cpp)
AbyssUI.dll — UI and main menu (mainui.cpp, ImGui files)
AbyssMod.dll — mod loading (modloader.cpp)
DATS.exe — thin executable that links against the DLLs (main.cpp)
Build System

Compile.py — incremental, multithreaded build system. TUI. Left-centered console UI. Saves 400-600 hours per year.
Compile.bat — legacy batch file. Embedded logic.
w64devkit — embedded toolchain. Fully offline. Static linking.
Clang — compiler. Cross-compiles to Windows, Linux, Android.
LLVM — backend. Used by Clang.
glslangValidator — shader compilation.
builds/ — output folder. Never compiles to Source/engine/.
No single massive executable. Modules compile to DLLs.
Cross-Platform

Windows — primary target.
Linux — Clang cross-compile.
Android — Clang from NDK.
iOS — no. Apple requires Metal and Xcode. Not supported.
macOS — no. Same reason.
Consoles — no. Sony, Microsoft, Nintendo require proprietary APIs. Not supported.
Renderer

Vulkan only. No OpenGL. No DirectX 12.
VK_PRESENT_MODE_MAILBOX — preferred. Uncapped FPS.
MAX_FRAMES_IN_FLIGHT = 2 — triple buffering caused queue backpressure on Intel.
D16_UNORM — depth format. Half bandwidth of D32.
Indirect draws — vkCmdDrawIndexedIndirect. 108 draw calls becomes 1.
Frustum culling — AABB p-vertex test. Extracted from unflipped projection.
No secondary command buffers — Intel driver crashes on parallel recording.
Serial primary recording — faster than parallel secondary.
Per-material batching — one vertex buffer, one index buffer, one indirect buffer.
Batched vkCmdExecuteCommands — 64 secondaries per batch. If re-added.
Vertex format — 32 bytes (pos vec3, normal vec3, uv vec2). Packed to 16 or 20 in future.
Descriptor management — single descriptor set with dynamic UBO offsets (planned).
Command buffer reuse — pre-record static geometry (planned).
GPU-driven culling — compute shaders (planned).
Textures

WebP — preferred format. Lossy better than JPEG. Lossless better than PNG. Transparency. Animation. Smallest files.
libwebp 1.6.0 — used for decoding. Headers must match library version.
cgltf — GLB parsing. Single header.
stb_image — PNG/JPEG fallback.
SVG — UI elements. 1KB for a logo. Infinite resolution. No blur.
WebM — video. Open container. VP9/AV1. Smaller than MP4.
No Draco — not compiling to WebGL. Not needed.
GLB — binary glTF. The JPEG of 3D. One file. Everything embedded.
No .mtl hell. No .obj. No hieroglyphics.
Audio

uLog — logging. Single header. Made for microcontrollers. Zero-cost when disabled.
Geiger system — 500GB of samples compressed to 10MB. Real recordings. Different models. Different isotopes.
Breakcore — custom. Old PC style. Windows 95 chimes. Dial-up. Nullscapes.
31 EDO — main menu theme. Microtonal. Stateless. No country.
Synthetic agony — voice replication. No actors. More realistic than ElevenLabs.
Breathing — temperature-based. Cold: shallow. Hot: heavy. Visible vapor.
Final exhale — when corpse is lifted. Not at death. Trapped air. Mechanical displacement.
Logging

Loggy — auth system and anticheat. Log format: [log/LOGGY] Take action now -> {description of case}
Real-time scanning — user-hosted servers send hashes. IWF database. Operator alerted.
24-hour grace period — operator must remove illegal content.
Escalation — law enforcement contacted if not removed.
Account banned — from mod page and official servers.
Server delisted — from public server list.
Evidence preserved — for law enforcement.
Privacy protected — only hashes sent. No chat. No voice. No gameplay. No config.
Anticheat

Server-authoritative — client says "I jumped." Server says "No."
Physics validation — gravity, momentum, collision.
Ballistics validation — barrel direction, bullet drop, travel time.
Movement validation — speed, acceleration, terrain.
Inventory validation — ammo count, weapon state.
Hit registration — position, angle, line of sight.
Occlusion — what can be seen from where.
No kernel access — no privacy concerns.
No bypass — server is authoritative.
No false positives — physics doesn't lie.
Ban config — operator sets threshold, duration, scope.
Appeal system — enabled by default.
Whitelist — trusted players exempt.
PHYSICS
Auxonoma

Autonoma + Axiom. A self-governing truth. A body that follows its own laws.
Not hitboxes. Axioms.
Unified solver — vehicles and bodies. Same code path. Tweaked values.
Bones, muscles, tendons, skin — rupture, tear, break.
Regeneration — S2. Agonising. Bones knitting. Muscles re-weaving.
Circulation — arterial vs venous. Pulsatile vs steady. Bright red vs dark red.
Respiration — chest rise and fall. Breathing rate. Final exhale.
Thermoregulation — body temperature. Heat loss. Ambient temperature.
Cardiac — heart rate. Pulse sync. Cardiac arrest.
Volume — blood volume loss. Shock. Unconsciousness.
Materials — metal, flesh, fabric, gas, concrete.
Failure modes — rupture, tear, break, ignite, collapse.
Air Physics

Buoyancy — hydrogen vs air density. Lift force.
Drag — air resistance on massive envelope.
Wind — external forces, gusts, turbulence.
Pressure — altitude effects on gas expansion.
Puncture — hydrogen leak rate based on hole size.
Ignition — fire propagation through envelope.
Structural integrity — frame stress, envelope tearing.
Mass distribution — center of gravity, balance.
Control surfaces — rudder, elevators, engine thrust.
Zeppelin — 250,000 cows per ship. Goldbeater's skin. Intestines. One 9mm Luger round. Fire. Fall. Death.
Vehicle Physics

Watertanks — called that in code. Reference to WWI. Hidden in plain sight.
Tanks, aircraft, FPV drones — same solver. Different materials.
DJI controller support — FPV drones. Real hardware integration.
Body Parts

All organs — heart, lungs, liver, kidneys, stomach, intestines.
Sexual organs — 100% realistic anatomy. Not random flesh.
Modders warned — adult content flagged. Segregated. Official servers clean.
GAME
D.A.T.S.

Dynamic Arms Training Simulator. Not a game. A simulation.
No flags. No bad guys. Just humans.
Every flag from every era during WWI upwards. Including Liechtenstein. British Raj. Czechoslovakia. Biafra. UN.
1914 to 2016. Every weapon. Every vehicle. Every aircraft.
Matches

24-hour matches — deployment, not match.
72-hour matches — endurance test.
200-hour matches — official. A war.
Persistent world — continues when you log off.
User-hosted servers — LAN, UPnP, direct IP.
UPnP embedded — port forwarding. No router config.
No central servers — communities run their own.
Maps

20km² — infantry.
200km² — combined arms.
500×500km — tournament maps. 250,000km². Larger than the UK.
Chunked terrain — 16x16 chunks, 16x16 quads each, 2000-unit world.
Greedy meshing — merges faces.
Batch processing — reduces overhead.
100,000,000+ dirt particles — most are culled.
2,000+ blades of grass — sampled, not textured.
Combat

No crosshair — barrel is the aim point. Crosshair is decorative.
No nametags — ribbons sometimes. Identification is the player's job.
Friendly fire — permanent. Kill a teammate, they're dead. Match is lost.
Instant black — death is instant. No ragdoll. No fade. No music.
Slow fade — MC's death. Earned. Watched.
Hunger and thirst — bodies need things. Bodies fail without them.
Radiation — hot but cold. Tingling. Numb. Unexplainable.
Weapons

81MB total — all weapons. 4K quality. GLB. WebP.
1914-2016 — real progression. Bolt-action to railgun.
Time-based rankings — 100 levels per era. 500 levels total.
New players — WWI weapons. Bolt-action. Bayonets.
Veterans — assault rifles. MBTs. Helicopters.
Legends — railguns. Exosuits. Autonomous drones.
Matchmaking — brackets by era. Mixed-era only if server operator chooses.
Tanks

Watertanks — in code. Hidden from pirates.
Armor penetration — realistic models.
Track physics — suspension. Terrain.
Crew positions — internal component damage.
Aeronautics

Zeppelin — near impossible to rank up in. One incendiary round and done.
Biplane — light, agile, fragile.
Fighter — speed, guns, dogfights.
Jet — speed, missiles, radar.
Helicopter — hover, precision.
Drone — remote, autonomous.
FPV — DJI controller support. Real hardware.
VR

Body suit support — Teslasuit XR5. Python API bridge.
Bones knitting — sequential pulses.
Muscles re-weaving — waves of vibration.
Tendons snapping — sharp, localized jolts.
Radiation fatigue — low, constant hum.
Hot but cold — radiation sensation. Unexplainable.
Playground

Advanced simulation space — nothing but a baseplate.
No objectives — no enemies — no Geiger meter.
Medical simulations — doctors train. Gunshot wounds. Arterial bleeds.
Car tests — crash tests. Rollovers. Structural failure.
Aeronautics — Zeppelin punctures. Jet stalls. Drone swarms.
Ballistics — penetration. Ricochet. Fragmentation.
Radiation — exposure. Contamination. Geiger response.
Structural — building collapse. Bridge failure. Demolition.
Thermal — fire spread. Explosions. Heat transfer.
Fluid — blood, fuel, water, coolant.
Crowd — panic, evacuation, stampede.
Modding

Custom programming language — parsed in Python, executed in C/ASM.
ABI never breaks — like mcaddon. Old mods work forever.
Optimized and renewed — old features get faster every year.
.dats — mod file. tar.gz. mod.json metadata. main.dats entry.
.datsmodp — modpack. tar.gz of tar.gz. modpack.json. load order.
Decompressed into RAM — not disk. Portable. %AppData%/.DATS/.
Shaderpack support — SPIR-V. shaderpack.json. Uniforms. Render passes.
In-game mod page — browse, preview, install, update, uninstall, rate, report, upload.
Content moderation — hash matching. IWF. Law enforcement contact.
Content segregation — official, community, private, playground.
Adult content flagging — legally required. Everywhere.
Vore banned — official servers only. User-hosted: their own rules.
UI

Dear ImGui — Vulkan backend.
Left-centered — console style.
Monomo — the font. Orphaned. Adopted. Named.
SVG icons — 1KB per icon. Infinite resolution.
Translucent — everything. Even MS Paint.
Dynamic Island — on the taskbar. Moved from floating.
Video wallpaper — animated. DATS. Monomo.
Config — %AppData%/Roaming/DATS/config.toml.
Monomo

Orphaned font. Found. Adopted. Named.
Like Mojangles — abandoned by Microsoft. Slowly diluted.
Monomo — kept. Protected. Used everywhere.
The name — one. Alone. Given purpose.
INFRASTRUCTURE
Distribution

Own domain — permanently owned. HTTPS.
No Steam — first few months. Maybe later.
No £76 Steam Direct fee — direct distribution.
No 30% cut — your revenue.
No rules — your moderation.
No acquisition — you stay independent.
$15-20 — game price.
Donations — optional. No P2W. No battle passes. No loot boxes.
Free updates — every version (year.month).
Free mods — modders share freely.
Free modpacks — .datsmodp files are free.
Free campaigns — S1 through S4 included.
Free multiplayer — user-hosted. No subscription.
Free Playground — sandbox included.
Free VR — medical training included.
Loggy Auth

Every server requires Loggy Auth.
Pirates can only see the Campaign.
Campaign: playable. Playground: playable. Multiplayer: no.
Official servers: no. User-hosted servers: no. Mod page: no.
Mod uploads: no. Updates: no. Support: no.
No DRM — no kernel access. No always-online for single-player.
Campaign is the gift. Multiplayer is the membership.
Login System

OpenSSL — PBKDF2 password hashing.
RAND_bytes — salt generation.
Session tokens — issued on login.
Server-side validation — token verified by game server.
No plaintext — passwords never stored.
SQLite or flat file — database.
HTTPS — your domain.
Rate limiting — prevent brute force.
Versioning

year.month — 2026.2, 2026.8, 2027.1.
Every release is a timestamp.
ABI never breaks — 2026.2 mods run in 2031.7.
Old features get optimized and renewed.
Content Moderation

Hash matching — IWF database. CSAM detection.
Real-time scan — user-hosted servers.
Operator alert — 24 hours to remove.
Escalation — law enforcement contacted.
Account banned — from mod page and official servers.
Server delisted — from public server list.
Evidence preserved — for law enforcement.
Privacy protected — only hashes sent.

