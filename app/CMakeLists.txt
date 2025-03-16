# Set the application name
set(APPLICATION_NAME "app")

# Add the app executable
add_executable(${APPLICATION_NAME} app.cpp)

# Link the iGe shared library
target_link_libraries(${APPLICATION_NAME} PRIVATE iGe)

# Set the output directory for the executable to ensure it can find the DLL
set_target_properties(${APPLICATION_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
