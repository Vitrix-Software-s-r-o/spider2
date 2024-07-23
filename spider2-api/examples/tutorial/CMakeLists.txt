function(add_spider2_example name)
    set(target spider2-example-${name})
    add_executable(${target} ${name}.cpp deps/json.cpp deps/easylogging.cpp)
    target_link_libraries(${target} PRIVATE spider2-api)
    if (SPIDER2_EXE_STATIC_RUNTIME)
        target_link_libraries(${target} PRIVATE -static -static-libgcc -static-libstdc++)
    endif ()

    #directory exists
    if (EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${name}/resources)
        #copy resources to build directory
        add_custom_command(TARGET ${target} POST_BUILD
                COMMAND ${CMAKE_COMMAND} -E copy_directory
                ${CMAKE_CURRENT_SOURCE_DIR}/${name}/resources
                $<TARGET_FILE_DIR:${target}>/resources)
    endif ()
endfunction()


add_spider2_example(intro)
