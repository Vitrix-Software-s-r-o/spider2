# libFuzzer harnesses for spider2 parsers.
#
# Enabled with -DSPIDER2_FUZZ=ON. Requires a Clang toolchain (libFuzzer) and is
# always built with ASan + UBSan, independent of the SPIDER2_SANITIZE_* options.
# The parsers are header-only, so only the harness translation units compile here.

if (NOT CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    message(FATAL_ERROR
        "SPIDER2_FUZZ requires a Clang toolchain for libFuzzer. "
        "Re-run cmake with -DCMAKE_C_COMPILER=clang -DCMAKE_CXX_COMPILER=clang++")
endif ()

set(spider2_fuzz_flags -g -O1 -fno-omit-frame-pointer -fsanitize=fuzzer,address,undefined)

function(spider2_add_fuzzer name source)
    add_executable(${name} ${source})
    target_compile_features(${name} PRIVATE cxx_std_20)
    target_compile_options(${name} PRIVATE ${spider2_fuzz_flags})
    target_link_options(${name} PRIVATE ${spider2_fuzz_flags})
    target_link_libraries(${name} PRIVATE fmt::fmt Microsoft.GSL::GSL ${Boost_LIBRARIES})
    target_include_directories(${name} PRIVATE
            ${CMAKE_SOURCE_DIR}/spider2/include
            ${Boost_INCLUDE_DIR})
endfunction()

spider2_add_fuzzer(fuzz_multipart fuzz_multipart.cpp)
spider2_add_fuzzer(fuzz_multipart_stream fuzz_multipart_stream.cpp)
