# Set the application name
set(APPLICATION_NAME "ExampleLayer")

# Add the app executable
add_executable(${APPLICATION_NAME} ExampleLayer.cpp)

# Link the iGe shared library
target_link_libraries(${APPLICATION_NAME} PRIVATE iGe)
set_property(TARGET ${APPLICATION_NAME} PROPERTY CXX_SCAN_FOR_MODULES 1)

# Set the output directory for the executable to ensure it can find the DLL
set_target_properties(${APPLICATION_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)
