#!/usr/bin/env bash
set -euo pipefail
root=$(cd -- "$(dirname -- "${BASH_SOURCE[0]}")/.." && pwd)
cd "$root"
python3 tools/prepare.py --fetch
git -C .deps/xenia submodule update --init --depth 1 --jobs 4 \
 third_party/xbyak third_party/capstone third_party/fmt third_party/cxxopts \
 third_party/tomlplusplus third_party/utfcpp third_party/disruptorplus \
 third_party/xxhash third_party/aes_128 third_party/imgui third_party/catch \
 third_party/date third_party/rapidjson third_party/tabulate third_party/rapidcsv third_party/SDL2 third_party/pugixml
bash .deps/PS5_Vulkan/tools/setup-native-dependencies.sh
bash .deps/PS5_Vulkan/tools/build-radv.sh release
