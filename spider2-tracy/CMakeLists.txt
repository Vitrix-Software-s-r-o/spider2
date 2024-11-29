set(target spider2-tracy)

set(spider2-tracy_Sources

)

if (NOT TARGET ${target})
    find_package(Tracy)
    if (NOT Tracy_FOUND)
        MESSAGE(STATUS "spider2-tracy: Tracy not found, skipping...")
    else ()
        add_library(${target} INTERFACE ${spider2-tracy_Sources})
        target_include_directories(${target} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
        MESSAGE(STATUS "spider2-tracy:${CMAKE_CURRENT_SOURCE_DIR}")

        target_link_libraries(${target} INTERFACE spider2 Tracy::TracyClient)
        target_compile_definitions(${target} INTERFACE TRACY_ENABLE)
        target_compile_definitions(${target} INTERFACE TRACY_FIBERS)
        target_compile_definitions(${target} INTERFACE TRACY_ON_DEMAND)

        add_executable(${target}-example examples/main.cpp)
        target_link_libraries(${target}-example ${target} ${EXE_LIBS})
    endif ()
endif ()
