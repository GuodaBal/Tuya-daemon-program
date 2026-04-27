set( DEMO_NAME "device_info_updater" )
set( DEMO_SOURCES "device_info.c" "become_daemon.c" "action_handler.c" "interface_info.c" "tuya_actions.c")

# Demo target.
add_executable(
    ${DEMO_NAME}
        "${DEMO_NAME}.c"
        ${DEMO_SOURCES}
)

target_link_libraries(
    ${DEMO_NAME}
    PUBLIC
        utils_modules
        platform_port
        middleware_implementation
        link_core
)

target_include_directories(
    ${DEMO_NAME}
    PUBLIC
        ${CMAKE_CURRENT_LIST_DIR}
)
