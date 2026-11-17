# Abyss Engine --- Work On

> **Status:** Pre-alpha / in-development / game-jam engine\
> **Purpose:** A blunt engineering backlog based on the reviewed
> repository snapshot.\
> **Scope:** Fix demonstrable blockers first. This is not a
> production-readiness checklist.

## How to use this document

-   [ ] Mark an item complete only after testing it.
-   [ ] Keep fixes small and independently buildable.
-   [ ] Record platform-specific behavior rather than claiming universal
    support.
-   [ ] Move intentionally deferred items to the Deferred section and
    explain why.
-   [ ] Do not implement everything here before making a game. The goal
    is to finish a playable game using the engine.

### Priorities

  -----------------------------------------------------------------------
  Priority                            Meaning
  ----------------------------------- -----------------------------------
  **P0 --- Blocker**                  Prevents building, launching,
                                      rendering, or using a core path.

  **P1 --- High**                     Likely to cause crashes, incorrect
                                      output, or major workflow friction.

  **P2 --- Medium**                   Correctness, maintainability,
                                      portability, or documentation
                                      improvement.

  **P3 --- Later**                    Polish or expansion that can wait
                                      until after the jam.
  -----------------------------------------------------------------------

------------------------------------------------------------------------

# 1. P0 --- Fix before relying on the engine

## 1.1 Main loop and menu integration

-   [x] **Integrate the menu render path into the application loop.**
    -   `source/main.cpp` now calls `Engine::Update()` and branches on
        `GameState` to call either `renderer.beginUIFrame()/RenderMenu()/endUIFrame()`
        (MENU, PAUSED) or `renderer.drawGameFrame()` (PLAYING).
    -   `source/engine.cpp::Update()` no longer double-renders menus.
-   [x] **Define an explicit engine/game state model.**
    -   `engine.h` declares `enum class GameState { MENU, PLAYING, PAUSED }`.
    -   The main loop in `main.cpp` queries this explicitly each frame.
-   [x] **Make the quit path reachable.**
    -   `glfwSetWindowShouldClose(window)` is called when
        `Engine::IsQuitRequested()` returns true or when a window-close
        callback fires (GLFW default handles close).
-   [x] **Test the main loop with a minimal scene.**
    -   Verified: build succeeds, menu renders in MENU state, game frame
        renders in PLAYING state via existing Vulkan swapchain.

## 1.2 BPC pipeline --- fixed interface mismatches

-   [x] **Fix the `classify` import.**
    -   `main.py` imports `classify_mesh` from `classify.py`. Aligned.
-   [x] **Fix `extract_positions()` unpacking.**
    -   `extract.py` yields `(pi, pos, idx, nrm, uv, material_idx)`.
    -   `main.py` now unpacks six values explicitly with named variables.
-   [x] **Fix the classifier return contract.**
    -   `classify_mesh()` returns `(kind, params)` where `params` is a dict.
    -   `main.py` wraps it as `{"component":0, "primitive":kind, "params":...}`
        consistently.
-   [x] **Add a BPC smoke test.**
    -   Added `source/bpc/tests.py`: runs a valid GLB through the full
        pipeline and validates output schema.
-   [x] **Add a malformed-input test.**
    -   `tests.py` tests missing file, invalid magic bytes, and empty
        scenes. All produce clear errors, not tracebacks.

## 1.3 Vulkan renderer correctness

-   [x] **Request every queue family used.**
    -   `createDevice()` now collects unique graphics and present families
        and requests both via separate `VkDeviceQueueCreateInfo` entries.
-   [x] **Handle swapchain queue-family sharing.**
    -   `createSwapchain()` uses `VK_SHARING_MODE_CONCURRENT` with both
        family indices when graphics ≠ present; exclusive otherwise.
-   [x] **Correct semaphore wait stage for overlay sampling.**
    -   Pipeline barrier already targets
        `VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT`.
-   [x] **Handle `VK_ERROR_OUT_OF_DATE_KHR`.**
    -   `drawFrame()` now calls `recreateSwapchain()` instead of silently
        returning.
-   [x] **Handle `VK_SUBOPTIMAL_KHR` and presentation errors.**
    -   Suboptimal triggers recreation on the next frame; other errors
        raise an exception.
-   [x] **Implement resize/minimize recovery.**
    -   Registered `glfwSetFramebufferSizeCallback` which sets a flag;
        `drawFrame()` recreates the swapchain when that flag is set.
        Nuklear context is re-initialized after recreation.
-   [x] **Check surface capabilities before selecting swapchain
    settings.**
    -   Validated extent clamping against min/max; zero-sized
        framebuffers throw instead of creating an unusable swapchain.

## 1.4 Build pipeline blockers

-   [x] **Remove the hard-coded Windows Python path from `Compile.py`.**
    -   Now reads `$PYTHON_ROOT` / `$DATS_PYTHON_ROOT`; falls back to the
        previously hard-coded path only if neither env-var is set.
-   [x] **Link CPython on Linux.**
    -   `LIBS_LINUX` appends `-L<PYTHON_LIB>` and `-lpython<VER>` when
        discovered.
-   [x] **Fail clearly when `glslc` is missing.**
    -   Replaced silent “skipping shaders” with `RuntimeError` that tells
        the user how to install the SDK or supply pre-built `.spv` files.
-   [x] **Validate Python development dependencies.**
    -   Print actionable setup instructions (apt / msi / embeddable
        package URL) when headers or lib are missing.
-   [ ] **Build from a clean checkout.**
    -   Do not depend on stale object files, generated shaders, or local
        SDK state.

------------------------------------------------------------------------

# 2. P1 --- High-value correctness and stability

## 2.1 Python embedding

-   [x] **Fix `PyMethodDef` pointer lifetime.**
    -   `source/python.cpp` now allocates each `PyMethodDef` individually
        with `new`, pushes a placeholder into the vector, and swaps in
        the stable allocation. This prevents vector reallocation from
        invalidating CPython's retained pointers.
-   [ ] **Document Python ownership and reference counting.**
    -   Identify borrowed vs. owned references.
    -   Release owned references on all success and failure paths.
-   [ ] **Centralize Python error handling.**
    -   Check C API return values and report Python exceptions rather
        than continuing with invalid state.
-   [ ] **Define GIL/threading policy before calling Python from
    multiple threads.**
    -   Worker threads must not enter Python without an
        initialization/GIL strategy.
    -   If calls are currently single-threaded, document that
        restriction.
-   [ ] **Add a minimal embedded-Python test.**
    -   Initialize interpreter, register a native method, call it,
        handle an exception, and test shutdown if supported.
-   [ ] **Document Python ownership and reference counting.**
    -   Identify borrowed vs. owned references.
    -   Release owned references on all success and failure paths.
-   [ ] **Centralize Python error handling.**
    -   Check C API return values and report Python exceptions rather
        than continuing with invalid state.
-   [ ] **Define GIL/threading policy before calling Python from
    multiple threads.**
    -   Worker threads must not enter Python without an
        initialization/GIL strategy.
    -   If calls are currently single-threaded, document that
        restriction.
-   [ ] **Add a minimal embedded-Python test.**
    -   Initialize interpreter, register a native method, call it,
        handle an exception, and test shutdown if supported.

## 2.2 Renderer and resource lifetime

-   [ ] **Audit Vulkan destruction order.**
    -   Ensure GPU work referencing an object has completed before
        destroying it.
    -   Destroy swapchain-dependent resources before the swapchain.
    -   Destroy device resources before the device and instance.
-   [ ] **Make resource creation fail atomically.**
    -   If creation fails halfway through, clean up already-created
        resources.
-   [ ] **Add Vulkan validation layers to development builds.**
    -   Do not require validation layers in release builds.
-   [ ] **Log Vulkan failures with operation names and result codes.**
-   [ ] **Test on at least two GPU/driver configurations if possible.**
    -   If only one is tested, document it as the tested configuration.

## 2.3 GLB/BPC data correctness

-   [x] **Use inverse-transpose for normals when needed.**
    -   `main.py` now computes the inverse-transpose of the world
        transform's upper-left 3×3 matrix before transforming normals.
        Singular transforms fall back to the identity.
-   [ ] **Define supported GLB features.**
    -   List supported primitive modes, accessors, component types,
        indices, materials, and transforms.
-   [x] **Validate accessor bounds and buffer offsets.**
    -   `glb_reader.py::read_accessor()` now checks:
      - accessor index against `accessors` length
      - computed buffer-view end offset against actual blob size
    -   Malformed input raises `ValueError` with a descriptive message.
-   [ ] **Handle normalized integer accessors or reject them explicitly.**
-   [ ] **Handle sparse accessors or reject them explicitly.**
-   [ ] **Handle external buffers or reject them explicitly.**
-   [ ] **Add test fixtures:** one mesh, multiple primitives,
    indexed/non-indexed geometry, normals, UVs, node transforms,
    nonuniform scale, and unsupported-feature cases.

------------------------------------------------------------------------

# 3. P2 --- Build reliability and developer experience

## 3.1 Build system

-   [ ] **Make compiler and SDK paths configurable.**
-   [ ] **Support paths containing spaces.**
    -   The reviewed dependency parsing splits `.d` files on whitespace,
        which can break paths.
    -   Use robust parsing/escaping.
-   [ ] **Make build errors actionable.**
    -   Report missing compiler, SDK, Python version, header, library,
        or shader compiler.
-   [ ] **Add a safe clean/rebuild command.**
-   [ ] **Separate source files, generated files, and third-party
    dependencies.**
-   [ ] **Document a known-good toolchain:** compiler/version, OS,
    Vulkan SDK, Python version, GLFW/GLAD/Nuklear dependencies, and
    shader compiler.
-   [ ] **Add a basic CI build once the project stabilizes.**
    -   Start with one known-good platform; expand only when
        maintainable.

## 3.2 Repository hygiene

-   [ ] **Review the 203.6 MiB tracked repository size.**
    -   Identify source, assets, SDKs, binaries, and generated outputs.
    -   Avoid committing full SDKs/generated binaries without a strong
        reason.
-   [ ] **Use submodules, package management, or documented dependency
    downloads where appropriate.**
-   [ ] **Preserve upstream license and notice files.**
-   [ ] **Add `.gitignore` entries for objects, build output, generated
    SPIR-V, caches, and local configuration.**
-   [ ] **Verify a clean checkout:** clone → install dependencies →
    configure → compile → launch → run BPC smoke test.

------------------------------------------------------------------------

# 4. P2 --- Documentation and claims

## 4.1 README accuracy

-   [ ] **Separate implemented features from planned features.**
    -   The README describes server-authoritative networking, UDP, a
        120--240 Hz server, physics/ballistics/fluid dynamics, x86-64
        assembly, UDM, and FPV integration.
    -   The reviewed first-party source tree does not show corresponding
        implementations for many of these.
    -   Label each as implemented, experimental, planned, or not
        implemented.
-   [ ] **Do not present planned performance or architecture as
    completed functionality.**
-   [ ] **Reconcile the Vulkan version claim.**
    -   README advertises Vulkan 1.3; reviewed renderer requests Vulkan
        API 1.0.
    -   Either raise the API version and validate requirements or
        correct the docs.
-   [ ] **Fix the Vulkan badge/version mismatch.**
-   [ ] **Create a dedicated Abyss Engine README.**
    -   The root README in the reviewed snapshot is primarily DATS game
        documentation.
    -   Document engine purpose, status, build instructions,
        architecture, and limitations.
-   [ ] **Add a current limitations section.**
    -   Supported OS/GPU/driver, renderer limitations, Python
        constraints, BPC support, and planned features.
-   [ ] **Add a minimal first-scene guide.**
    -   Build, launch, load/create a scene, render an object, and handle
        input.
-   [ ] **Tie screenshots and feature claims to actual builds.**

## 4.2 Licensing

-   [ ] **Explain DSAL 1.0 accurately.**
    -   The reviewed license is source-available and noncommercial; it
        is not OSI-approved open source.
    -   Do not call the project "open source" without clarifying its
        license restrictions.
-   [ ] **Preserve third-party license notices.**
-   [ ] **Review license compatibility before redistributing
    dependencies.**
-   [ ] **Seek legal advice if the license is intended to govern
    commercial use, contributions, or redistribution.**
    -   This checklist is not legal advice.

------------------------------------------------------------------------

# 5. P3 --- Performance work (after correctness)

**Measure first. Do not optimize based on intuition.**

-   [ ] Add CPU frame-time instrumentation: update, render submission,
    UI, Python/BPC.
-   [ ] Add GPU timing where supported.
-   [ ] Track average frame time, 1% low, and worst spikes in a
    repeatable scene.
-   [ ] Create a benchmark scene with fixed camera, geometry,
    resolution, and settings.
-   [ ] Profile to identify CPU, GPU, memory, Python, or synchronization
    bottlenecks.
-   [ ] Avoid multithreading before ownership and synchronization are
    clear.
-   [ ] Avoid custom allocators or custom GC-like systems without
    measured need.
-   [ ] Test on actual low-end hardware before claiming low-end
    optimization.
-   [ ] Record CPU, GPU, RAM, OS, resolution, and settings for
    benchmarks.
-   [ ] Define minimum hardware from test results, not aspiration.

------------------------------------------------------------------------

# 6. P3 --- Architecture and feature expansion

These are not immediate game-jam blockers.

-   [ ] Define module boundaries: core, renderer, window/input, UI,
    Python, asset/BPC pipeline, and game code.
-   [ ] Keep game logic out of engine internals where practical.
-   [ ] Define ownership for long-lived resources.
-   [ ] Define threading rules before adding worker systems.
-   [ ] Define the asset pipeline: source format, import step, runtime
    format, cache, and versioning.
-   [ ] Define a consistent error-reporting convention.
-   [ ] Avoid exposing unstable internals as public API too early.
-   [ ] Version serialized asset formats if they must survive engine
    changes.
-   [ ] Build a small vertical slice before expanding networking,
    physics, or simulation.

------------------------------------------------------------------------

# 7. Game-jam milestone --- finish a playable game

The engine's value is not measured by how many systems its README lists.
It is measured by whether it enables a game to be completed.

## Minimum milestone

-   [ ] Engine builds from a clean checkout on the intended machine.
-   [ ] Window opens and closes correctly.
-   [ ] Main menu or clear entry point works.
-   [ ] Scene renders consistently.
-   [ ] Player input works.
-   [ ] One gameplay loop works from start to finish.
-   [ ] Restart works.
-   [ ] Errors are visible enough to debug.
-   [ ] Game can be packaged and launched by someone other than the
    developer.
-   [ ] The game is completed before optional engine features are added.

## Suggested vertical slice

1.  Open window.
2.  Render one scene.
3.  Read input.
4.  Move one object/player.
5.  Detect one gameplay interaction.
6.  Show UI feedback.
7.  Reach a win or lose condition.
8.  Restart.
9.  Package and test.

-   [ ] Complete this slice before adding complex networking, advanced
    physics, or broad asset-format support.

------------------------------------------------------------------------

# 8. Testing checklist

## Every build

-   [ ] Clean compile succeeds.
-   [ ] Application launches.
-   [ ] Window resizes.
-   [ ] Window minimizes and restores.
-   [ ] Application closes without hanging.
-   [ ] No obvious Vulkan validation errors in development mode.
-   [ ] No missing shader or asset errors.

## Renderer

-   [ ] Scene renders at intended resolution.
-   [ ] UI/overlay appears correctly.
-   [ ] Resize does not break rendering.
-   [ ] Synchronization is validation-clean where validation is
    available.
-   [ ] Swapchain recreation works after resize/out-of-date status.

## Python

-   [ ] Interpreter initializes.
-   [ ] Native method registration works.
-   [ ] Python calls a native function.
-   [ ] Python errors are reported.
-   [ ] Shutdown is tested or explicitly unsupported.
-   [ ] Threading restrictions are documented.

## BPC

-   [ ] Valid GLB imports.
-   [ ] Expected geometry is emitted.
-   [ ] Transform and normal handling pass tests.
-   [ ] Unsupported features produce clear errors.
-   [ ] Malformed input does not crash the engine.

## Game

-   [ ] Entry point works.
-   [ ] Gameplay starts and finishes.
-   [ ] Restart works.
-   [ ] Package works in a clean environment.

------------------------------------------------------------------------

# 9. Deferred --- not required for pre-alpha/game jam

-   [ ] Production-grade GPU/device compatibility.
-   [ ] Broad CI matrix across every OS and GPU.
-   [ ] Complete glTF/GLB specification support.
-   [ ] Stable public plugin ABI.
-   [ ] Advanced networking architecture.
-   [ ] Complete physics, ballistics, and fluid simulation.
-   [ ] Large-scale multithreaded job system.
-   [ ] Advanced memory allocator.
-   [ ] Formal performance guarantees.
-   [ ] Extensive automated test coverage.
-   [ ] Production crash reporting/telemetry.
-   [ ] Full editor tooling.
-   [ ] Comprehensive asset hot-reloading.
-   [ ] Every feature currently listed in the README.

**Deferred does not mean forgotten.** It means the feature has not
earned priority over finishing the current game.

------------------------------------------------------------------------

# 10. Hard truths / engineering rules

-   [ ] **A class or README section is not proof that a feature works.**
    It is implemented when the end-to-end behavior works.
-   [ ] **Do not advertise support that has not been tested.**
-   [ ] **Do not hide build failures by skipping required steps.**
-   [ ] **Do not add features to compensate for broken fundamentals.**
-   [ ] **Do not optimize before measuring.**
-   [ ] **Do not expand supported formats without tests.**
-   [ ] **Do not confuse a large codebase with a mature engine.**
-   [ ] **Pre-alpha is not permission to ignore correctness.**
-   [ ] **Game-jam scope is not production scope.**
-   [ ] **Make the engine serve the game---not the game serve the
    engine.**

------------------------------------------------------------------------

# Recommended order

1.  [ ] Main loop/menu integration.
2.  [ ] BPC import and return-contract mismatches.
3.  [ ] Vulkan queue selection and synchronization.
4.  [ ] Swapchain error/resize handling.
5.  [ ] Clean-build fixes: Python linkage and shader compiler checks.
6.  [ ] Python method-definition lifetime.
7.  [ ] Minimal renderer, Python, and BPC tests.
8.  [ ] README status and setup documentation.
9.  [ ] Complete a playable game-jam vertical slice.
10. [ ] Profile performance and prioritize based on measured
    bottlenecks.

**Final priority:** make a reliable vertical slice, then improve the
engine based on the problems that slice exposes.
