#!/bin/bash
set -euo pipefail
cd "$(dirname "$0")/.."

if [[ "$(uname -s)" != Darwin || "$(uname -m)" != arm64 ]]; then
  printf '%s\n' 'This script builds the native Apple Silicon port on an ARM64 Mac.' >&2
  exit 1
fi

# Restore only the dependencies used by the native build, at their gitlink pins.
git submodule update --init --depth 1 --jobs "${JOBS:-4}" -- \
  third_party/aes_128 third_party/capstone third_party/catch \
  third_party/cxxopts third_party/date third_party/discord-rpc \
  third_party/disruptorplus third_party/FFmpeg third_party/fmt \
  third_party/imgui third_party/pugixml third_party/rapidcsv \
  third_party/rapidjson third_party/SDL2 third_party/snappy \
  third_party/tabulate third_party/tomlplusplus third_party/utfcpp \
  third_party/xbyak third_party/xbyak_aarch64 third_party/xxhash \
  third_party/zarchive third_party/zlib-ng third_party/zstd \
  third_party/DirectX-Headers third_party/DirectXShaderCompiler-mac \
  third_party/metal-cpp third_party/metal-shader-converter \
  third_party/SPIRV-Cross third_party/glslang third_party/Vulkan-Headers \
  third_party/FidelityFX-CAS third_party/FidelityFX-FSR

build_dir="${BUILD_DIR:-build-macos-arm64}"
cmake -S . -B "$build_dir" -G Ninja \
  -DCMAKE_TOOLCHAIN_FILE="$PWD/tools/toolchains/macos-arm64.cmake" \
  -DCMAKE_BUILD_TYPE=Release -DXENIA_BUILD_TESTS=ON \
  -DXENIA_METAL_SHADER_CONVERTER=ON "$@"
cmake --build "$build_dir" --target xenia-app xenia-base-tests xenia-cpu-tests \
  --parallel "${JOBS:-4}"
printf 'Application: %s/%s/bin/macOS/xenia_canary.app\n' "$PWD" "$build_dir"
