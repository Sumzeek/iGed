set(TARGET_NAME ${SHADER_COMPILE_TARGET})

file(GLOB SHADER_FILES CONFIGURE_DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/slang/*.slang")
SOURCE_GROUP("shaders" FILES ${SHADER_FILES})

# Output directory: directly to binary assets
set(SHADER_OUTPUT_DIR "${CMAKE_BINARY_DIR}/bin/assets/shaders")

# Target platforms
set(SHADER_PLATFORMS "dxil;spirv")

# Compile shaders
include("${IGED_ROOT_DIR}/Cmake/ShaderCompiler.cmake")
CompileShaders(
        "${TARGET_NAME}"
        "${CMAKE_CURRENT_SOURCE_DIR}/tools"
        "${SHADER_FILES}"
        "${SHADER_OUTPUT_DIR}"
        "${SHADER_PLATFORMS}"
)
