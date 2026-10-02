#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$root"
python3 tools/prepare.py
python3 tools/xenia-overlay.py build/ps5/xenia-overlay
vulkan="$root/.deps/PS5_Vulkan"
sdk="$vulkan/.deps/native/ps5-payload-sdk"
work="$root/build/ps5"
native="$vulkan/tooling/native"
tool="$vulkan/build/runtime-shim/ps5-native-tool"
archive="$vulkan/.deps/native/radv-release/lib/libvulkan_radeon.ps5.a"
[[ -f $tool && -f $vulkan/runtime/libc.prx ]] || bash "$vulkan/tools/rebuild-libc.sh"
mkdir -p "$work/obj" "$work/stubs"
cmake -S tools/xenia-core -B build/xenia-ps5 -G Ninja -DX360_TARGET_PS5=ON \
  -DCMAKE_TOOLCHAIN_FILE="$root/tools/ps5-toolchain.cmake"
cmake --build build/xenia-ps5 -j 4
glslangValidator -V --target-env vulkan1.1 --vn probe_shader shaders/probe.comp -o "$work/generated/probe_shader.hpp"
cc() { PS5_PAYLOAD_SDK="$sdk" sh "$vulkan/tooling/prospero-clang18" "$@"; }
objects=()
for file in diagnostics platform exception_probe xenia_memory_probe gpu_probe ps5_main; do
    object="$work/obj/$file.o"
    cc -std=c++20 -O2 -fexceptions -fcxx-exceptions -fno-rtti -ffunction-sections -fdata-sections -DX360_PS5=1 \
      -I "$work/xenia-overlay/src" -I "$root/.deps/xenia/src" -iquote "$root/.deps/xenia/src/xenia/base" -I "$root/.deps/xenia" \
      -I "$root/.deps/PS5_Mesa/include" -I "$work/generated" -I "$root/src" \
      -c "$root/src/$file.cpp" -o "$object"
    objects+=("$object")
done
objects+=("$root/build/xenia-ps5/CMakeFiles/xenia-probe-entry.dir$root/src/xenia_core_probe.cpp.o")
cc -std=c++20 -O2 -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections \
  -I "$work/generated" -c "$work/generated/demo_renderer.cpp" -o "$work/obj/renderer.o"
for file in app_crt app_cpp_runtime; do
    cc -std=c++20 -O2 -fno-exceptions -fno-rtti -ffunction-sections -fdata-sections \
      -c "$native/$file.cpp" -o "$work/obj/$file.o"
done
for pair in libSceAgc:agc_canary_link_stub libSceAgcDriver:agc_driver_canary_link_stub; do
    library=${pair%%:*}; source=${pair#*:}
    cc -std=c11 -O2 -fPIC -c "$vulkan/vendor/ps5/sdk/stubs/$source.c" -o "$work/obj/$library.o"
    "$sdk/bin/prospero-lld" --shared -soname "$library.prx" -o "$work/stubs/$library.so" "$work/obj/$library.o"
done
if [[ ${1:-} == --objects-only ]]; then
  echo 'PS5 objects compiled; linking not requested.'
  exit 0
fi
[[ -f $archive ]] || { echo 'Build RADV first: bash .deps/PS5_Vulkan/tools/build-radv.sh release' >&2; exit 2; }
source "$vulkan/tools/radv-link.sh"
radv_link_recipe "$vulkan" "$sdk" "$archive"
"$sdk/bin/prospero-lld" "${radv_linker_script[@]}" --eh-frame-hdr "${radv_link_flags[@]}" \
  --version-script "$native/app-symbols.map" --exclude-libs=ALL --gc-sections -e _start \
  -o "$work/llvm-pie.elf" "$work/obj/app_crt.o" "$work/obj/app_cpp_runtime.o" \
  "$work/obj/renderer.o" "${objects[@]}" "$work/stubs/libSceAgc.so" "$work/stubs/libSceAgcDriver.so" \
  "$root/build/xenia-ps5/libxenia-probe-core.a" "$root/build/xenia-ps5/fmt/libfmt.a" \
  "${radv_link_inputs[@]}" --as-needed "$sdk"/target/lib/*.so
"$tool" link --in "$work/llvm-pie.elf" --out "$work/eboot.elf" --stub-dir "$sdk/target/lib" \
  --stub "$work/stubs/libSceAgc.so" --stub "$work/stubs/libSceAgcDriver.so" \
  --module-sdk 0x02000009 --companion-sdk 0x08050001 --file-name eboot.elf
app="$root/dist/PPSA99361"
mkdir -p "$app/sce_sys" "$app/sce_module"
"$tool" self --sign --in "$work/eboot.elf" --out "$app/eboot.bin" --magic 0x1D3D154F
cp "$work/generated/param.json" "$app/sce_sys/param.json"
cp "$root/.deps/boilerplate/sce_sys/icon0.png" "$app/sce_sys/icon0.png"
cp "$vulkan/runtime/libc.prx" "$app/sce_module/libc.prx"
"$tool" self --inspect --file "$app/eboot.bin" > "$work/inspection.txt"
"$tool" self --inspect --file "$app/sce_module/libc.prx" >> "$work/inspection.txt"
echo "PS5 diagnostic built: $app (hardware untested)"
