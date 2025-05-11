# Set the application name
set(APPLICATION_NAME "SandboxApp")

# Add the app executable
add_executable(${APPLICATION_NAME}
        src/SandboxApp.cpp
        src/ExampleLayer.cpp
)

# Link the iGe shared library
target_link_libraries(${APPLICATION_NAME} PRIVATE iGe)
set_property(TARGET ${APPLICATION_NAME} PROPERTY CXX_SCAN_FOR_MODULES 1)

# Set the output directory for the executable to ensure it can find the DLL
set_target_properties(${APPLICATION_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Define the source and destination for the assets
set(ASSETS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set(ASSETS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/assets")

# Custom command to copy assets after build
add_custom_command(TARGET ${APPLICATION_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${ASSETS_SOURCE_DIR}"
        "${ASSETS_OUTPUT_DIR}"
)
