#!/usr/bin/env bash
# Build the multipart parser fuzzer with clang + libFuzzer + ASan + UBSan.
set -euo pipefail
cd "$(dirname "$0")/.."

VCPKG_INC="cmake-build-debug/vcpkg_installed/x64-linux/include"

clang++ -std=c++20 -g -O1 -fno-omit-frame-pointer \
  -fsanitize=fuzzer,address,undefined \
  -I spider2/include \
  -I "$VCPKG_INC" \
  fuzz/fuzz_multipart.cpp \
  -o fuzz/fuzz_multipart

echo "built fuzz/fuzz_multipart"
