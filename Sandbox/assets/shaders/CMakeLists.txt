set(TARGET_NAME ${SHADER_COMPILE_TARGET})

file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/slang/*.slang")
SOURCE_GROUP("shaders" FILES ${SHADER_FILES})

# Set generated shader folder name
set(GENERATED_SHADER_FOLDER "generated")
if (NOT EXISTS ${CMAKE_CURRENT_SOURCE_DIR}/${GENERATED_SHADER_FOLDER})
    file(MAKE_DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR}/${GENERATED_SHADER_FOLDER})
endif ()

# Set final output directory for compiled assets
set(ASSETS_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/assets")
set(COPY_DIR "${ASSETS_OUTPUT_DIR}/shaders")

# Compile shaders for multiple platforms
include("${IGED_ROOT_DIR}/Cmake/ShaderCompiler.cmake")
CompileToPlatforms(
        "${TARGET_NAME}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools"
        "${SHADER_FILES}"
        "${CMAKE_CURRENT_SOURCE_DIR}/${GENERATED_SHADER_FOLDER}"
        "${COPY_DIR}"
)