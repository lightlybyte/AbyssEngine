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

-   [ ] **Integrate the menu render path into the application loop.**
    -   `source/main.cpp` calls `Engine::Update()` and
        `Engine::RenderGame()`.
    -   `Engine::RenderMenu()` exists in `source/engine.cpp`, but the
        reviewed main loop does not call it.
    -   Verify the intended state flow: menu → game → pause/menu → exit.
-   [ ] **Define an explicit engine/game state model.**
    -   Avoid scattered booleans and implicit state assumptions.
    -   Ensure every state has update and render behavior.
-   [ ] **Make the quit path reachable.**
    -   Verify window close and in-game exit both terminate cleanly.
-   [ ] **Test the main loop with a minimal scene.**
    -   Confirm update, render, input, and transitions happen in the
        expected order.

## 1.2 BPC pipeline --- immediate interface mismatches

These appear to prevent the BPC pipeline from running. They are
functional blockers, not style issues.

-   [ ] **Fix the `classify` import.**
    -   `source/bpc/main.py` imports `classify`.
    -   `source/bpc/classify.py` defines `classify_mesh()` and
        `report()`, not `classify()`.
    -   Choose one API and align the import/call.
-   [ ] **Fix `extract_positions()` unpacking.**
    -   `extract.py` returns six values:
        `(pi, pos, idx, nrm, uv, material_idx)`.
    -   `main.py` unpacks four.
    -   Align caller and callee; document any intentionally discarded
        fields.
-   [ ] **Fix the classifier return contract.**
    -   `classify_mesh()` returns `(kind, params)`.
    -   The caller expects dictionaries with fields such as `component`,
        `primitive`, and `params`.
    -   Establish one schema and use it consistently.
-   [ ] **Add a BPC smoke test.**
    -   Run a minimal valid GLB through the complete pipeline.
    -   Verify output and geometry.
-   [ ] **Add a malformed-input test.**
    -   Unsupported or malformed input should produce a clear error, not
        an opaque traceback or partial output.

## 1.3 Vulkan renderer correctness

-   [ ] **Request every queue family used.**
    -   `findQueueFamilies()` can select distinct graphics and present
        families.
    -   The reviewed device creation path requests only the graphics
        family, then retrieves graphics and present queues.
    -   If the families differ, the present queue was not requested.
    -   Request both families or require a single family supporting
        both.
-   [ ] **Handle swapchain queue-family sharing.**
    -   The swapchain defaults to exclusive sharing.
    -   With separate graphics/present families, implement ownership
        transfers or use concurrent sharing.
-   [ ] **Correct semaphore wait stage for overlay sampling.**
    -   The compositor samples the UI overlay in a fragment shader.
    -   The reviewed wait stage is
        `VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT`.
    -   Ensure the wait protects fragment-shader sampling. Validate the
        full dependency chain; do not blindly change the mask.
-   [ ] **Handle `VK_ERROR_OUT_OF_DATE_KHR`.**
    -   Recreate the swapchain after acquisition reports out-of-date.
    -   Do not simply return and risk a stuck renderer.
-   [ ] **Handle `VK_SUBOPTIMAL_KHR` and presentation errors.**
    -   Check `vkQueuePresentKHR` results and recreate when necessary.
-   [ ] **Implement resize/minimize recovery.**
    -   Test resizing, minimizing, restoring, and changing window
        dimensions.
    -   Do not recreate a swapchain for a zero-sized framebuffer.
-   [ ] **Check surface capabilities before selecting swapchain
    settings.**
    -   Validate surface format, image count, composite alpha, extent,
        and present mode.

## 1.4 Build pipeline blockers

-   [ ] **Remove the hard-coded Windows Python path from `Compile.py`.**
    -   Use configuration, environment variables, or documented
        discovery.
    -   A clean build must not depend on one developer's machine.
-   [ ] **Link CPython on Linux.**
    -   The reviewed `LIBS_LINUX` configuration does not include the
        Python library despite `source/python.cpp` using the CPython C
        API.
    -   Confirm the actual linker command and fix the missing
        dependency.
-   [ ] **Fail clearly when `glslc` is missing.**
    -   Shader compilation is skipped if `glslc` is unavailable, while
        the renderer expects compiled SPIR-V.
    -   Require `glslc`, provide a valid precompiled-shader workflow, or
        stop with a useful error.
-   [ ] **Validate Python development dependencies.**
    -   Check headers, import/shared library, and runtime version.
    -   Print actionable setup instructions if missing.
-   [ ] **Build from a clean checkout.**
    -   Do not depend on stale object files, generated shaders, or local
        SDK state.

------------------------------------------------------------------------

# 2. P1 --- High-value correctness and stability

## 2.1 Python embedding

-   [ ] **Fix `PyMethodDef` pointer lifetime.**
    -   `source/python.cpp` stores method definitions in a
        `std::vector`, reserves `size + 1`, pushes entries, and passes
        pointers to CPython.
    -   CPython retains method-definition pointers. A later vector
        reallocation can invalidate earlier pointers.
    -   Use stable storage such as `std::deque`, individually allocated
        definitions, or a known maximum reserved before registration.
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

-   [ ] **Use inverse-transpose for normals when needed.**
    -   `main.py` transforms normals with `world_M[:3, :3]`, which is
        incorrect under nonuniform scaling.
    -   Normalize transformed normals and handle singular transforms.
-   [ ] **Define supported GLB features.**
    -   List supported primitive modes, accessors, component types,
        indices, materials, and transforms.
-   [ ] **Handle normalized integer accessors or reject them
    explicitly.**
-   [ ] **Handle sparse accessors or reject them explicitly.**
-   [ ] **Handle external buffers or reject them explicitly.**
-   [ ] **Validate accessor bounds and buffer offsets.**
    -   Malformed input must not cause out-of-bounds reads.
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
