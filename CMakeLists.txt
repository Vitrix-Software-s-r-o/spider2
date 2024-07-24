cmake_minimum_required(VERSION 3.20)
project(spider2 CXX)

option(SPIDER2_ENABLE_EXAMPLES "Enable building of examples" OFF)
option(SPIDER2_EXE_STATIC_RUNTIME "Use static runtime" OFF)
option(SPIDER2_COMPRESS "Spider2 compress middleware" OFF)
option(SPIDER2_TRACY "Tracy middleware for" OFF)
option(SPIDER2_TESTS "Enable building of tests" OFF)


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
