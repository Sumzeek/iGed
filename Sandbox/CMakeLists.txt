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

# Add third party denpendency
add_subdirectory(3rdparty)
target_link_libraries(${TARGET_NAME} PUBLIC assimp::assimp)
target_link_libraries(${TARGET_NAME} PUBLIC Eigen3::Eigen)

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

# Copy textures
file(GLOB_RECURSE TEXTURE_FILES CONFIGURE_DEPENDS "${ASSETS_SOURCE_DIR}/textures/*")
SOURCE_GROUP("textures" FILES ${TEXTURE_FILES})

set(COPIED_TEXTURE_FLAG "${ASSETS_OUTPUT_DIR}/textures/.copy")
add_custom_command(
        OUTPUT ${COPIED_TEXTURE_FLAG}
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${ASSETS_SOURCE_DIR}/textures"
        "${ASSETS_OUTPUT_DIR}/textures"
        COMMAND ${CMAKE_COMMAND} -E touch "${COPIED_TEXTURE_FLAG}" # create dummy file
        DEPENDS ${TEXTURE_FILES}
        COMMENT "Copying textures..."
        VERBATIM
)
add_custom_target(CopyTexturesTarget DEPENDS ${COPIED_TEXTURE_FLAG})
add_dependencies(${TARGET_NAME} CopyTexturesTarget)

# Copy shaders
file(GLOB_RECURSE SHADER_FILES CONFIGURE_DEPENDS "${ASSETS_SOURCE_DIR}/shaders/generated/*")
SOURCE_GROUP("shaders" FILES ${SHADER_FILES})

set(COPIED_SHADER_FLAG "${ASSETS_OUTPUT_DIR}/shaders/.copy")
add_custom_command(
        OUTPUT ${COPIED_SHADER_FLAG}
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${ASSETS_SOURCE_DIR}/shaders/generated"
        "${ASSETS_OUTPUT_DIR}/shaders"
        COMMAND ${CMAKE_COMMAND} -E touch "${COPIED_SHADER_FLAG}" # create dummy file
        DEPENDS ${SHADER_FILES}
        COMMENT "Copying shaders..."
        VERBATIM
)
add_custom_target(CopyShadersTarget DEPENDS ${COPIED_SHADER_FLAG})
add_dependencies(${TARGET_NAME} CopyShadersTarget)

# Copy models
file(GLOB_RECURSE MODEL_FILES CONFIGURE_DEPENDS "${ASSETS_SOURCE_DIR}/models/*")
SOURCE_GROUP("models" FILES ${MODEL_FILES})

set(COPIED_MODEL_FLAG "${ASSETS_OUTPUT_DIR}/models/.copy")
add_custom_command(
        OUTPUT ${COPIED_MODEL_FLAG}
        COMMAND ${CMAKE_COMMAND} -E copy_directory
        "${ASSETS_SOURCE_DIR}/models"
        "${ASSETS_OUTPUT_DIR}/models"
        COMMAND ${CMAKE_COMMAND} -E touch "${COPIED_MODEL_FLAG}" # create dummy file
        DEPENDS ${MODEL_FILES}
        COMMENT "Copying shaders..."
        VERBATIM
)
add_custom_target(CopyModelsTarget DEPENDS ${COPIED_MODEL_FLAG})
add_dependencies(${TARGET_NAME} CopyModelsTarget)
