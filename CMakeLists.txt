cmake_minimum_required(VERSION 3.20)
project(spider2 CXX)

add_compile_options(-fno-omit-frame-pointer)

option(SPIDER2_EXAMPLES "Enable building of examples" OFF)

# this is enables return brotli compression middleware
option(SPIDER2_COMPRESS "Spider2 compress middleware" OFF)

# this is integrates with Tracy profiler with feature fibers enabled
option(SPIDER2_TRACY "Tracy middleware for" OFF)
option(SPIDER2_TESTS "Enable building of tests" OFF)



option(SPIDER2_SANITIZE_ADDRESS "Enable address sanitizer" OFF)
if (SPIDER2_SANITIZE_ADDRESS)
    add_compile_options(-fsanitize=address)
    add_link_options(-fsanitize=address)
endif ()

option(SPIDER2_SANITIZE_UNDEFINED "Enable undefined sanitizer" OFF)
if (SPIDER2_SANITIZE_UNDEFINED)
    add_compile_options(-fsanitize=undefined)
    add_link_options(-fsanitize=undefined)
endif ()

option(SPIDER2_SANITIZE_THREAD "Enable thread sanitizer" OFF)
if (SPIDER2_SANITIZE_THREAD)
    add_compile_options(-fsanitize=thread)
    add_link_options(-fsanitize=thread)
endif ()


# this is a private Vitrix Software, s.r.o. feature
option(SPIDER2_INTEGRATE_VITRIX_SERIALIZATION "Enable building of Vitrix Software serialization library" OFF)

if (NOT DEFINED CMAKE_CXX_STANDARD)
    MESSAGE(STATUS "CMAKE_CXX_STANDARD NOT SET: DEFAULTING TO C++20")
    set(CMAKE_CXX_STANDARD 20)
else ()
    MESSAGE(DEBUG "CMAKE_CXX_STANDARD IS SET: C++${CMAKE_CXX_STANDARD}")
endif ()

find_path(CPP_BASE64_INCLUDE_DIRS "cpp-base64/base64.cpp")
find_package(Boost 1.79.0 REQUIRED COMPONENTS system filesystem program_options date_time iostreams)
find_package(fmt CONFIG REQUIRED)
find_package(Microsoft.GSL CONFIG REQUIRED)
find_package(Catch2 CONFIG REQUIRED)

if (SPIDER2_TESTS)
    enable_testing()
endif ()

add_subdirectory(spider2)
add_subdirectory(spider2-api)

add_subdirectory(spider2-testing)
add_subdirectory(spider2-prometheus)
add_subdirectory(spider2-proxy)
add_subdirectory(spider2-react)

if (SPIDER2_COMPRESS)
    add_subdirectory(spider2-compress)
endif ()

if (SPIDER2_TRACY)
    add_subdirectory(spider2-tracy)
endif()


