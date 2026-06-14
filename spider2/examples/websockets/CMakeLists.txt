set(target spider2-websockets-example)
add_executable(${target} main.cpp site/index.html)
target_link_libraries(${target} spider2 ${EXE_LIBS})
target_include_directories(${target} PRIVATE ${Boost_INCLUDE_DIR})

add_custom_command(TARGET ${target} POST_BUILD
        WORKING_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}
        COMMAND ${CMAKE_COMMAND} -E copy_directory site $<TARGET_PROPERTY:${target},BINARY_DIR>/site
        COMMENT "Copying test-site to target directory of ${target}"
)