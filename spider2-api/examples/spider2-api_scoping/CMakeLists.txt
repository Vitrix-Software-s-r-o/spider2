set(target spider2-api_scoping)

add_executable(${target} main.cpp index.html
        json.cpp)
target_link_libraries(${target} PRIVATE spider2-api)

#copy index.html to build directory
add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/index.html
        $<TARGET_FILE_DIR:${target}>/resources/index.html)

#copy Dockerfile to build directory
add_custom_command(TARGET ${target} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/Dockerfile
        $<TARGET_FILE_DIR:${target}>/Dockerfile)