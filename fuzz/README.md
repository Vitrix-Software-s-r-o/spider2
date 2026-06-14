# Fuzzing `multipart_message_parser`

libFuzzer harness for `spider2::multipart_message_parser` (header-only, so the
fuzzer compiles standalone with clang — no need to build the rest of spider2).

## Build (CMake)

Fuzzing is gated behind `-DSPIDER2_FUZZ=ON` and requires a Clang toolchain
(libFuzzer). The harness is always built with ASan + UBSan, regardless of the
`SPIDER2_SANITIZE_*` options.

```bash
cmake -S . -B build-fuzz \
  -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++ \
  -DSPIDER2_FUZZ=ON \
  -DCMAKE_TOOLCHAIN_FILE=<path-to>/vcpkg/scripts/buildsystems/vcpkg.cmake
cmake --build build-fuzz --target fuzz_multipart
```

The parsers are header-only, so only the harness translation unit is compiled —
the rest of the project is untouched. Binary lands at
`build-fuzz/fuzz/fuzz_multipart`.

### Quick standalone build (no CMake)

`fuzz/build.sh` compiles the harness directly with clang against the vcpkg
headers under `cmake-build-debug/vcpkg_installed/` — handy for a fast edit/run
loop without reconfiguring CMake.

## Run

```bash
./build-fuzz/fuzz/fuzz_multipart -max_total_time=60 fuzz/corpus   # time-boxed
./build-fuzz/fuzz/fuzz_multipart fuzz/corpus                      # run until crash
./build-fuzz/fuzz/fuzz_multipart -jobs=8 -workers=8 fuzz/corpus   # parallel
```

### Input format

The fuzzer needs both a boundary and a body, so the harness splits the input:

```
[1 byte: boundary length N][N bytes: boundary][rest: multipart body]
```

## Reproducing / minimizing a crash

```bash
./fuzz/fuzz_multipart fuzz/crashes/empty-boundary-length-error      # reproduce
./fuzz/fuzz_multipart -minimize_crash=1 -runs=100000 <crash-file>   # minimize
```

## Known findings

- **`crashes/empty-boundary-length-error`** — an **empty boundary** (e.g.
  `Content-Type: multipart/form-data; boundary=`) drives `it` past `end` in
  `on_possible_boundary`, after which `on_process_buffer` builds a `std::string`
  with a negative (→ huge) length and throws `std::length_error`. UBSan also flags
  out-of-bounds reads in `advance_if_equals` along the way. Minimal repro is 5
  bytes: empty boundary + body `\r\n--`. Since the boundary is attacker-controlled
  via the request `Content-Type`, this is a remote crash / DoS.

## Ideas for more coverage

- A second harness that splits the body at a fuzzer-chosen offset and calls
  `on_data` in chunks, to exercise the streaming / re-buffering path.
- Seed the corpus with the bodies from `spider2/tests/multipart_parser_tests.cpp`.
