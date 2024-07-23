set(target spider2-react)

set(spider2-react_Sources
        include/spider2/react.h
)

if (NOT TARGET ${target})
    find_package(Tracy REQUIRED)
    add_library(${target} INTERFACE ${spider2-react_Sources})
    target_include_directories(${target} INTERFACE ${CMAKE_CURRENT_SOURCE_DIR}/include)
    MESSAGE(STATUS "spider2-react:${CMAKE_CURRENT_SOURCE_DIR}")

    target_link_libraries(${target} INTERFACE spider2-proxy)

    add_executable(${target}-example examples/main.cpp)
    target_link_libraries(${target}-example ${target} ${EXE_LIBS})
endif ()

option(SPIDER2_REACT_ENABLE_REACT_APP_BUILD "Enable building of examples" ON)

function(spider2_react_add_react_app_npm target react_app_dir)
    add_custom_command(TARGET ${target} POST_BUILD
            WORKING_DIRECTORY ${react_app_dir}
            COMMAND npm install
            COMMENT "Installing react app dependencies"
    )

    add_custom_command(TARGET ${target} POST_BUILD
            WORKING_DIRECTORY ${react_app_dir}
            COMMAND npm run build
            COMMENT "Building react app"
    )

    add_custom_command(TARGET ${target} POST_BUILD
            WORKING_DIRECTORY ${react_app_dir}
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${react_app_dir}/build $<TARGET_PROPERTY:${target},BINARY_DIR>/react_build
            COMMENT "Copying react app build to target directory of ${target}"
    )
endfunction()

function(spider2_react_add_react_app_yarn target react_app_dir)
    add_custom_command(TARGET ${target} POST_BUILD
            WORKING_DIRECTORY ${react_app_dir}
            COMMAND yarn
            COMMENT "Installing react app dependencies"
    )

    add_custom_command(TARGET ${target} POST_BUILD
            WORKING_DIRECTORY ${react_app_dir}
            COMMAND yarn build
            COMMENT "Building react app"
    )

    add_custom_command(TARGET ${target} POST_BUILD
            WORKING_DIRECTORY ${react_app_dir}
            COMMAND ${CMAKE_COMMAND} -E copy_directory ${react_app_dir}/build $<TARGET_PROPERTY:${target},BINARY_DIR>/react_build
            COMMENT "Copying react app build to target directory of ${target}"
    )
endfunction()

if (SPIDER2_REACT_ENABLE_REACT_APP_BUILD)
    if (NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
        spider2_react_add_react_app_npm(${target}-example
                ${CMAKE_CURRENT_SOURCE_DIR}/examples/example_app)
    endif ()
endif ()