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

## Harnesses

- **`fuzz_multipart`** — feeds the whole body in one `on_data()` call. Input format:
  `[1 byte N][N-byte boundary][body]`. Corpus: `fuzz/corpus/`.
- **`fuzz_multipart_stream`** — feeds the body across many `on_data()` calls at
  fuzzer-chosen chunk sizes, exercising the streaming / re-buffering path
  (`on_process_buffer` firing when the 8096-byte buffer fills, then
  `buffer_to_front`). Input format:
  `[1 byte N][N-byte boundary][1 byte K][K-byte chunk-size plan][body]`.
  Corpus: `fuzz/corpus_stream/`.
  **Run it with a large `-max_len`** (≥ 65536) — the buffer-fill path is only
  reached when the total body exceeds 8096 bytes.

## Run

```bash
# single-shot
./build-fuzz/fuzz/fuzz_multipart -max_total_time=60 fuzz/corpus

# streaming — note the large max_len so the internal buffer actually fills
./build-fuzz/fuzz/fuzz_multipart_stream -max_len=65536 -max_total_time=60 fuzz/corpus_stream

# parallel across all cores (fork mode), drop -ignore_crashes so the crash counter is real
./build-fuzz/fuzz/fuzz_multipart_stream -fork=$(nproc) -max_len=65536 \
    -rss_limit_mb=2048 -timeout=25 -max_total_time=3600 fuzz/corpus_stream
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

## Known findings (all fixed)

- **`on_data` span over-read (streaming).** When a body arrives across multiple
  `on_data()` calls and the internal buffer fills mid-chunk, the copy length was
  bounded by the whole span instead of the unconsumed remainder, reading past the
  input buffer (heap-buffer-overflow). Found by `fuzz_multipart_stream`; regression
  seed `corpus_stream/regress-onstream-overread` + a deterministic Catch2 case.
- **`advance_if_equals` / `on_possible_boundary` over-advance.** A partial delimiter
  at the buffer end falsely matched and advanced the cursor past `end`, and the
  candidate-skip `++it` could step past `end` — yielding OOB reads and a
  `std::length_error`. Seeds: `corpus/regress-*`.
- **Empty boundary** accepted and never made progress; now rejected up front.

### History

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
