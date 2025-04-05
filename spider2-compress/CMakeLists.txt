set(target spider2-compress)

set(spider2_spider2-compress_Sources
        include/spider2/middleware/compress.h
        src/compress.cpp
)

if (NOT TARGET ${target})
    find_package(unofficial-brotli CONFIG REQUIRED)

    add_library(${target} STATIC ${spider2_spider2-compress_Sources})
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)
    MESSAGE(STATUS "spider2-compress:${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(${target} PUBLIC spider2 spider2-testing)
    target_link_libraries(${target} PUBLIC unofficial::brotli::brotlienc unofficial::brotli::brotlidec unofficial::brotli::brotlicommon)

    if (SPIDER2_TESTS)
        add_executable(${target}-tests tests/main.cpp tests/compresss_tests.cpp)
        target_link_libraries(${target}-tests PUBLIC ${target} spider2-testing ${EXE_LIBS} Catch2::Catch2WithMain)
        target_include_directories(${target}-tests PRIVATE ${Boost_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
        add_test(NAME ${target}-tests COMMAND ${target}-tests)
    endif ()
endif ()