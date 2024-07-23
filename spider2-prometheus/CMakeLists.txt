set(target spider2-prometheus)

set(spider2_prometheus_Sources
        spider2/prometheus.h
        spider2/prometheus_parser.h

        src/prometheus.cpp
        src/prometheus_parser.cpp

)

if (NOT TARGET ${target})
    add_library(${target} STATIC ${spider2_prometheus_Sources}
    )
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR})
    target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
    target_link_libraries(${target} PUBLIC spider2)

    if (SPIDER2_TESTS)
    add_executable(${target}-tests tests/main.cpp tests/prometheus_tests.cpp
            tests/prometheus_parser_tests.cpp)
    target_link_libraries(${target}-tests ${target} catch ${EXE_LIBS})
    target_include_directories(${target}-tests PRIVATE ${Boost_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    endif ()
endif ()