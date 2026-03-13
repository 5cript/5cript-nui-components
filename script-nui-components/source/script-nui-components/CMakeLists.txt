add_library(
    script-nui-components
    STATIC
        "resizeable_table.cpp"
        "dialog.cpp"
)

target_include_directories(script-nui-components PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../../include")

target_link_libraries(
    script-nui-components
    PUBLIC
        nui-frontend
)