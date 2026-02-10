cmake_minimum_required(VERSION 3.21)

project(script-nui-components VERSION 0.1.0)

if (EMSCRIPTEN)
    add_subdirectory(${CMAKE_SOURCE_DIR}/script-nui-components/source/script-nui-components)
endif()

if (PROJECT_IS_TOP_LEVEL)
    # Nui Dependency
    add_subdirectory("${CMAKE_CURRENT_LIST_DIR}/dependencies/Nui")
    include (${CMAKE_CURRENT_LIST_DIR}/_cmake/common_options.cmake)

    # Add executable (sources are added later, depending on frontend/backend)
    add_executable(${PROJECT_NAME}-test)
    target_link_libraries(${PROJECT_NAME}-test PUBLIC core-target)

    set(PREJS_FILE ${CMAKE_SOURCE_DIR}/frontend/source/frontend/js/module.js)

    if (EMSCRIPTEN)
        add_subdirectory(${CMAKE_SOURCE_DIR}/frontend/source/frontend)
    else()
        add_subdirectory(${CMAKE_SOURCE_DIR}/backend/source/backend)

        # If msys2, copy dynamic libraries to executable directory, visual studio does this automatically.
        # And there is no need on linux.
        if (DEFINED ENV{MSYSTEM})
            add_custom_command(TARGET ${PROJECT_NAME}-test POST_BUILD
                COMMAND bash -c "ldd $<TARGET_FILE:${PROJECT_NAME}-test>" | "grep" "clang" | awk "NF == 4 { system(\"${CMAKE_COMMAND} -E copy \" \$3 \" $<TARGET_FILE_DIR:${PROJECT_NAME}-test>\") }"
                VERBATIM
            )
        endif()
    endif()
endif()