target_sources(${PROJECT_NAME}-test PRIVATE main.cpp main_page.cpp)

target_include_directories(${PROJECT_NAME}-test PRIVATE ${CMAKE_SOURCE_DIR}/frontend/include)

# Link frontend of nui inside of emscripten
target_link_libraries(${PROJECT_NAME}-test PRIVATE nui-frontend script-nui-components)

target_compile_definitions(${PROJECT_NAME}-test PRIVATE -DSCRIPT_NUI_COMPONENTS_NO_INLINE)

set(DEPENDANT_LINK_OPTIONS "")
set(DEPENDANT_COMPILE_OPTIONS "")
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    set(DEPENDANT_LINK_OPTIONS "-g1")
    set(DEPENDANT_COMPILE_OPTIONS "-g1")
else()
    set(DEPENDANT_LINK_OPTIONS "-O3")
    set(DEPENDANT_COMPILE_OPTIONS "-O3")
endif()

nui_prepare_emscripten_target(
    TARGET
        ${PROJECT_NAME}-test
    PARCEL_ARGS
        --no-optimize
    # Provide this argument if you dont want the main application bundled.
    # This can be useful if it gets large and you want to show a load screen.
    # LEAN_INDEX_HTML
    NPM_INSTALL_ARGS
        # --verbose
    PREJS
        ${PREJS_FILE}
    EMSCRIPTEN_LINK_OPTIONS
        -sEXPORTED_FUNCTIONS=_frontendMain
        -sALLOW_MEMORY_GROWTH=1
        -sWASM_BIGINT
        -fexceptions
        -sASSERTIONS=1
        ${DEPENDANT_LINK_OPTIONS}
        # -g
        # or other options, see emscripten docs.
    EMSCRIPTEN_COMPILE_OPTIONS
        -fexceptions
        ${DEPENDANT_COMPILE_OPTIONS}
        # -g
    CXX_STANDARD
        23
    OBSERVED_BUNDLE_FILES
        "${CMAKE_SOURCE_DIR}/static/styles/main.css"
        "${CMAKE_SOURCE_DIR}/styles/switch.css"
        "${CMAKE_SOURCE_DIR}/styles/text_input.css"
        "${CMAKE_SOURCE_DIR}/styles/select.css"
        "${CMAKE_SOURCE_DIR}/styles/button.css"
        "${CMAKE_SOURCE_DIR}/styles/color_picker.css"
        "${CMAKE_SOURCE_DIR}/styles/resizeable_table.css"
        "${CMAKE_SOURCE_DIR}/styles/dialog.css"
        "${CMAKE_SOURCE_DIR}/styles/tabs.css"
        "${CMAKE_SOURCE_DIR}/styles/popup_menu.css"
)