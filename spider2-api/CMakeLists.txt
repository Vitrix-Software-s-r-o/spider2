set(target spider2-api)

set(spider2-api_Sources
        include/spider2/api.h
        include/spider2/api/from_body.h
        include/spider2/api/from_query.h
        include/spider2/api/result/formatter.h
        include/spider2/api/result/is_api_result.h

        include/spider2/api/op_result/flat_value.h
        include/spider2/api/op_result/op_field_msg.h
        include/spider2/api/op_result/op_result.h
        include/spider2/api/op_result/op_msg.h
        include/spider2/api/op_result.h
        include/spider2/api/from_query/parser.h
        include/spider2/api/from_query/query_args.h
        include/spider2/api/from_query/from_query.h
        include/spider2/api/from_body/from_body.h
        include/spider2/api/from_body/parser.h
        include/spider2/api/support/config.h
        include/spider2/api/support/boost_json/support_impl.h
        include/spider2/api/support/boost_json/support_decl.h
        include/spider2/api/support/boost_json/use_boost_json.h

)

if (NOT TARGET ${target})
    add_library(${target} INTERFACE ${spider2-api_Sources})
    target_include_directories(${target} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    MESSAGE(STATUS "spider2-api:${CMAKE_CURRENT_SOURCE_DIR}")
    target_link_libraries(${target} INTERFACE spider2)

    if (SPIDER2_INTEGRATE_VITRIX_SERIALIZATION)
        if (TARGET commons)
            #enables support for vitrix commons serialization library
            target_link_libraries(${target} INTERFACE commons)
            target_compile_definitions(${target} INTERFACE SPIDER2_API_USE_VITRIX_COMMONS)
        else ()
            message(FATAL_ERROR "Vitrix serialization is enabled but commons library is not found")
        endif ()
    endif ()

    if (SPIDER2_TESTS)
        add_executable(${target}-tests
                tests/main.cpp
                tests/result_tests.cpp
                tests/op_result_tests.cpp
                tests/query_args_tests.cpp
                tests/old_spider_serialization_test.cpp
                tests/api_args_value_passing_tests.cpp
        )

        target_link_libraries(${target}-tests ${target} spider2-testing ${EXE_LIBS})
        target_include_directories(${target}-tests PRIVATE ${Boost_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})
    endif ()
endif ()

if (SPIDER2_EXAMPLES)
    add_subdirectory(examples)
endif ()