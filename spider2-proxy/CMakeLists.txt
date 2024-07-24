set(target spider2-proxy)

set(spider2-proxy_Sources
    include/spider2/proxy.h
)

if (NOT TARGET ${target})
    find_package(Tracy REQUIRED)
    add_library(${target} INTERFACE ${spider2-proxy_Sources})
    target_include_directories(${target} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    MESSAGE(STATUS "spider2-proxy:${CMAKE_CURRENT_SOURCE_DIR}")

    target_link_libraries(${target} INTERFACE spider2)
    IF (SPIDER2_EXAMPLES)
        add_executable(${target}-example examples/main.cpp)
        target_link_libraries(${target}-example ${target} ${EXE_LIBS})
    endif ()

endif ()
