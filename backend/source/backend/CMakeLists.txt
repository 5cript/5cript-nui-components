target_sources(${PROJECT_NAME}-test PRIVATE main.cpp)

target_include_directories(${PROJECT_NAME}-test PRIVATE ${CMAKE_SOURCE_DIR}/backend/include)

# Link backend of nui outside of emscripten
target_link_libraries(${PROJECT_NAME}-test PRIVATE nui-backend)

nui_set_target_output_directories(${PROJECT_NAME}-test  )

# Creates a target that is compiled through emscripten. This target becomes the frontend part.
nui_add_emscripten_target(
    TARGET
        ${PROJECT_NAME}-test
    PREJS
        ${PREJS_FILE}
    SOURCE_DIR
        ${CMAKE_SOURCE_DIR}
    CMAKE_OPTIONS
        # I recommend to work with a release build by default because debug builds get big fast.
        -DCMAKE_BUILD_TYPE=Debug
        # -DCMAKE_BUILD_TYPE=Release
)