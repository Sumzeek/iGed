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

# Dynamic compile shaders
set(SHADER_COMPILE_TARGET "iGedShaderCompiler")
add_subdirectory(assets/shaders)
add_dependencies(${TARGET_NAME} ${SHADER_COMPILE_TARGET})

# Set the output directory for the executable to ensure it can find the DLL
set_target_properties(${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"
)

# Define the source and destination for the assets
set(ASSETS_SOURCE_DIR "${CMAKE_CURRENT_SOURCE_DIR}/assets")
set(ASSETS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/assets")

# Copy textures
file(GLOB_RECURSE TEXTURE_FILES CONFIGURE_DEPENDS "${ASSETS_SOURCE_DIR}/textures/*")
SOURCE_GROUP("textures" FILES ${TEXTURE_FILES})

# Create output directory if it doesn't exist
file(MAKE_DIRECTORY "${ASSETS_OUTPUT_DIR}/textures")

set(COPIED_TEXTURE_FILES "")
foreach (TEXTURE_FILE ${TEXTURE_FILES})
    # Get relative path from textures directory
    file(RELATIVE_PATH RELATIVE_PATH "${ASSETS_SOURCE_DIR}/textures" "${TEXTURE_FILE}")
    # Create corresponding output path
    set(OUTPUT_FILE "${ASSETS_OUTPUT_DIR}/textures/${RELATIVE_PATH}")
    list(APPEND COPIED_TEXTURE_FILES ${OUTPUT_FILE})

    # Get directory of output file
    get_filename_component(OUTPUT_DIR "${OUTPUT_FILE}" DIRECTORY)

    # Add custom command to copy individual file
    add_custom_command(
            OUTPUT "${OUTPUT_FILE}"
            COMMAND ${CMAKE_COMMAND} -E make_directory "${OUTPUT_DIR}"
            COMMAND ${CMAKE_COMMAND} -E copy_if_different "${TEXTURE_FILE}" "${OUTPUT_FILE}"
            DEPENDS "${TEXTURE_FILE}"
            COMMENT "Copying texture: ${RELATIVE_PATH}"
            VERBATIM
    )
endforeach ()

add_custom_target(CopyTexturesTarget DEPENDS ${COPIED_TEXTURE_FILES})
add_dependencies(${TARGET_NAME} CopyTexturesTarget)
