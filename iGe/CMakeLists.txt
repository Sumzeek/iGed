set(TARGET_NAME iGe)

# Create a shared library (DLL)
add_library(${TARGET_NAME} SHARED)

# Set source files (all .ixx files in the src directory)
file(GLOB_RECURSE MODULE_SOURCES "modules/*.ixx")
target_sources(${TARGET_NAME} PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES ${MODULE_SOURCES}
)

# Set source files (all .cpp files in the src directory)
file(GLOB_RECURSE SOURCES "modules/*.cpp")
target_sources(${TARGET_NAME} PUBLIC ${SOURCES})

# Add third party denpendency
add_subdirectory(3rdparty)
target_link_libraries(${TARGET_NAME} PUBLIC spdlog::spdlog)
target_link_libraries(${TARGET_NAME} PUBLIC glfw)
target_link_libraries(${TARGET_NAME} PUBLIC glad)
target_link_libraries(${TARGET_NAME} PUBLIC imgui)
target_link_libraries(${TARGET_NAME} PUBLIC glm::glm)
target_link_libraries(${TARGET_NAME} PUBLIC stb_image)

# Set export symbols for the library
target_compile_definitions(${TARGET_NAME} PRIVATE IGE_EXPORT)

# set platform symbol
if (WIN32)
    target_compile_definitions(${TARGET_NAME} PUBLIC IGE_PLATFORM_WINDOWS)
else ()
    message(FATAL_ERROR "Unknown Platform: iGe only supports Windows!")
endif ()

# Add IGE_DEBUG in Debug mode
if (CMAKE_BUILD_TYPE STREQUAL "Debug")
    target_compile_definitions(${TARGET_NAME} PUBLIC IGE_DEBUG)
endif ()

# Set include directories
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Set output directories for the library
set_target_properties(${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"  # DLL files
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"  # Shared library files
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"  # Static library files
)
