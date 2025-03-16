set(TARGET_NAME iGe)

# Set source files (all .cpp files in the src directory)
file(GLOB SOURCES "src/iGe/*.cpp" "src/Platform/Windows/*.cpp")

# Create a shared library (DLL)
add_library(${TARGET_NAME} SHARED ${SOURCES})

# Enable precompiled headers (PCH)
target_precompile_headers(${TARGET_NAME} PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/include/iGepch.h)

# Add third party denpendency
add_subdirectory(3rdparty)
target_link_libraries(${TARGET_NAME} PUBLIC spdlog::spdlog)
target_link_libraries(${TARGET_NAME} PUBLIC glfw)
target_link_libraries(${TARGET_NAME} PUBLIC opengl32.lib)

# Set export symbols for the library
target_compile_definitions(${TARGET_NAME} PRIVATE IGE_EXPORT)

# set platform symbol
if (WIN32)
    target_compile_definitions(${TARGET_NAME} PUBLIC IGE_PLATFORM_WINDOWS)
else ()
    message(FATAL_ERROR "Unknown Platform: iGe only supports Windows!")
endif ()

# Set include directories
target_include_directories(${TARGET_NAME} PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Set output directories for the library
set_target_properties(${TARGET_NAME} PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"  # DLL files
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"  # Shared library files
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"  # Static library files
)
