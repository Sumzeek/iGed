# Set the application name
set(TARGET_NAME "SandboxApp")

# Add the app executable
add_executable(${TARGET_NAME} src/SandboxApp.cpp)

# Set source files (all .ixx files in the src directory)
file(GLOB_RECURSE MODULE_SOURCES "src/*.ixx")
target_sources(${TARGET_NAME} PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES ${MODULE_SOURCES}
)
# Set source files (all .cpp files in the src directory)
file(GLOB_RECURSE SOURCES "src/*.cpp")
target_sources(${TARGET_NAME} PUBLIC ${SOURCES})

# Link the iGe shared library
target_link_libraries(${TARGET_NAME} PRIVATE iGe)
set_property(TARGET ${TARGET_NAME} PROPERTY CXX_SCAN_FOR_MODULES 1)

# Compile shaders
set(SHADER_COMPILE_TARGET "iGedShaderCompile")
add_subdirectory(assets/shaders)
add_dependencies(${TARGET_NAME} ${SHADER_COMPILE_TARGET})

# Set the output directory for the executable to ensure it can find the DLL
set_target_properties(${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Define the source and destination for the assets
set(ASSETS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set(ASSETS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/assets")

# Custom command to copy assets after build
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${ASSETS_SOURCE_DIR}/textures"
        "${ASSETS_OUTPUT_DIR}/textures"
)
add_custom_command(TARGET ${TARGET_NAME} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${ASSETS_SOURCE_DIR}/shaders/generated/glsl"
        "${ASSETS_OUTPUT_DIR}/shaders/glsl"
)
