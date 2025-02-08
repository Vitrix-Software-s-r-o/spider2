set(target spider2)

set(spider2_Sources
        include/spider2/asio.h
        include/spider2/server.h
        include/spider2/types/platform/http.h
        include/spider2/types/structs/endpoint.h
        include/spider2/routing/endpoint_handler.h
        include/spider2/routing/router.h
        include/spider2/routing/nested_router.h
        include/spider2/routing/api_endpoint/api_endpoint.h
        include/spider2/concepts/middleware.h
        include/spider2/types.h
        include/spider2/types/platform/std_types.h
        include/spider2/types/platform/asio.h
        include/spider2/types/platform.h
        include/spider2/types/results.h
        include/spider2/types/structs/response.h
        include/spider2/types/structs/request.h
        include/spider2/types/structs/mime_table.h
        include/spider2/types/utils/overload.h
        include/spider2/types/utils.h
        include/spider2/types/utils/variant_cast.h
        include/spider2/types/utils/static_lookup_table.h
        include/spider2/types/structs/request_error_code.h
        include/spider2/types/structs/app_context_base.h
        include/spider2/middleware/thread_pool_middleware.h
        include/spider2/types/utils/middleware_chain.h
        include/spider2/middleware/make_async_execution_handler.h
        include/spider2/types/utils/any_function_adapter.h
        include/spider2/types/utils/get_arg.h
        include/spider2/spider2.h
        include/spider2/types/utils/async_file_io.h
        include/spider2/handlers/static_files.h
        src/cache_control.cpp
        include/spider2/routing.h
        include/spider2/types/structs/byte_range.h
        src/byte_range.cpp
        include/spider2/types/utils/http_date.h
        include/spider2/types/utils/ignore_result.h
        include/spider2/types/utils/etag.h
        src/etag.cpp
        src/http_date.cpp
        include/spider2/types/results/return_file.h
        include/spider2/types/utils/resource_status.h
        include/spider2/handlers.h include/spider2/types/utils/tuple_algorithms.h
        include/spider2/middleware/allow_origin_middleware.h
        include/spider2/middleware/cache_middleware.h
        include/spider2/types/structs/cache_backend.h
        include/spider2/types/structs/memory_cache_backend.h
        src/memory_cache_backend.cpp
        src/app_context_base.cpp
        include/spider2/types/utils/tag_invoke.h
        include/spider2/handlers/sync_handler.h
        include/spider2/routing/api_endpoint/read_api_request.h
        include/spider2/routing/api_endpoint/parse_api_request.h
        include/spider2/routing/api_endpoint/format_api_response.h
        src/base64.h
        src/base64.cpp
        include/spider2/handlers/websocket.h
        include/spider2/types/structs/websocket_connection.h
        include/spider2/types/structs/websocket_event_handler.h
        include/spider2/types/structs/message_queue.h
        include/spider2/types/structs/cancellation_timer.h

)

if (CMAKE_SYSTEM_NAME STREQUAL "Windows")
    set(spider2_OS
            src/windows/async_file_io.cpp
            src/windows/ignore_sig_pipe.cpp)
else ()
    set(spider2_OS
            src/posix/async_file_io.cpp
            src/posix/ignore_sig_pipe.cpp)
endif ()

if (NOT TARGET ${target})
    add_library(${target} STATIC ${spider2_Sources} ${spider2_OS})
    target_link_libraries(${target} PUBLIC fmt::fmt Microsoft.GSL::GSL ${Boost_LIBRARIES})

    target_include_directories(${target} PUBLIC ${Boost_INCLUDE_DIR})
    target_include_directories(${target} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    target_include_directories(${target} PRIVATE ${CPP_BASE64_INCLUDE_DIRS})
    target_include_directories(${target} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

    add_executable(${target}-example main.cpp)
    target_link_libraries(${target}-example spider2 ${EXE_LIBS})
    target_include_directories(${target}-example PRIVATE ${Boost_INCLUDE_DIR})

    if (SPIDER2_TESTS)
        add_executable(${target}-tests
                tests/utils_tests.cpp
                tests/request_routing_tests.cpp
                tests/mime_table_tests.cpp
                tests/async_file_tests.cpp
                tests/tag_invoke_tests.cpp
                tests/api_request_tests.cpp
                tests/tuple_tests.cpp
        )

        target_link_libraries(${target}-tests ${Boost_LIBRARIES} ${target} spider2-testing Catch2::Catch2WithMain ${EXE_LIBS})
        target_include_directories(${target}-tests PRIVATE ${Boost_INCLUDE_DIR} ${CMAKE_CURRENT_SOURCE_DIR})

    endif ()
endif ()