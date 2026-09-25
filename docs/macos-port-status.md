# macOS port integration

Working branch: `macos-canary-integration`.

Base: Canary `74c4e4acbad21bb16c2266fcff7d6420c6a8fc19`.
Platform references: wmarti macOS `f19e7dbb9ab6fa564769bdbe6a3c929ae32dd768`
and `v0.2.1` (`0b5b65227f2e300fcd6a7862bcdb037b39a5f7df`).

The objective is an Apple Silicon application using the current Canary CPU,
kernel and GPU core, with a native Metal backend. Fable II ISO is the game
validation target. macOS x86_64/Rosetta and Windows/CrossOver are acceptable
comparison or fallback paths, but are not the initial build target.

## Milestones

- [x] Preserve the original source archives and create an isolated Git worktree.
- [x] Identify and pin the upstream and platform source revisions.
- [x] Restore required submodules and configure an ARM64 build using CMake.
- [x] Compile and test macOS memory, threading, file and exception handling.
- [x] Execute the modern ARM64 JIT with macOS memory protection and ABI rules.
- [x] Integrate Cocoa UI and Metal with current Canary interfaces.
- [x] Build reproducible shaders and package/sign a standalone application.
- [x] Verify native application startup and GPU work on this Mac.
- [x] Fable II and Forza Horizon playable (see README).

## Build preparation

The macOS toolchain explicitly selects the target architecture and invokes
the native Apple compiler even if CMake itself runs under Rosetta:

```sh
cmake -S . -B build-macos-arm64 -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/tools/toolchains/macos-arm64.cmake" \
  -DCMAKE_BUILD_TYPE=Release -DXENIA_BUILD_TESTS=ON
cmake --build build-macos-arm64 --parallel 4
```

SDL2 and compression libraries are built from the pinned source dependencies.
Build products and generated headers belong to the build directory. Tests that
execute JIT code must be signed with the application's JIT entitlements.

## Validation record

Validated on Apple M4 / macOS 27.0 with AppleClang 21 (2026-09-23):

- Release ARM64 application builds, bundles converter libraries and is ad-hoc signed.
- Base tests: 82 cases, 3,740 assertions passed outside the execution sandbox.
- CPU tests: 267 cases, 973 assertions passed, including indirect calls,
  lazy compilation and unwinding through actual JIT frames.
- Metal: 85 embedded shaders compiled, 80 compute pipelines created, zero
  failures; GPU computation/readback matched all 64 values.
- Standalone bundle: ../dist/Xenia Canary.app is ARM64 and passes strict
  signature verification. A second 25-second launch confirmed both converter
  libraries loaded from its own Frameworks directory after relocation.
- Startup: Cocoa loop, Metal 1280×720 surface, GPU and audio threads, immediate
  drawer and DXBC→DXIL→Metal converter initialized. The initial smoke test
  stayed running for 25 seconds and was stopped intentionally.

These results do not establish game compatibility, frame rate, audio quality
or save behavior. CrossOver and x86_64/Rosetta were not used or validated.

Build instructions: ../README.md. Pinned dependencies:
macos-source-lock.json. The separate macOS DXC fork receives the reproducible
SDK compatibility patch in cmake/patches/dxilconv-macos.patch.

Full Xcode is unnecessary for this build;
Apple Command Line Tools, CMake, Ninja and Python 3.9+ are required.
Minimum deployment target is macOS 15.0; runtime tested on 27.0 only.
