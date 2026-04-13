add_library(
    script-nui-components
    STATIC
        "resizeable_table.cpp"
        "dialog.cpp"
        "tabs.cpp"
        "popup_menu.cpp"
        "tree.cpp"
        "dropdown_menu.cpp"
        "color_picker.cpp"
        "button.cpp"
        "message_strip.cpp"
        "switch.cpp"
        "text_input.cpp"
        "carousel.cpp"
)

target_include_directories(script-nui-components PUBLIC "${CMAKE_CURRENT_LIST_DIR}/../../include")

target_link_libraries(
    script-nui-components
    PUBLIC
        nui-frontend
        ui5-sap-icons
)