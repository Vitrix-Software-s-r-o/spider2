set(target spider2-testing)

set(spider2_spider2-tesging_Sources
        include/spider2/testing.h
        include/spider2/testing/process_request.h
)

if (NOT TARGET ${target})
    add_library(${target} INTERFACE ${spider2_spider2-tesging_Sources})
    target_include_directories(${target} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)

    MESSAGE(STATUS "spider2-testing:${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(${target} INTERFACE spider2 Catch2::Catch2)
endif ()