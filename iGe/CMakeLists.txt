set(TARGET_NAME iGe)

# Set source files (all .cpp files in the src directory)
file(GLOB SOURCES "src/iGe/*.cpp")

# Create a shared library (DLL)
add_library(${TARGET_NAME} SHARED ${SOURCES})

# Add third party denpendency
add_subdirectory(3rdparty)
set(3rdparty_dependency)
list(APPEND 3rdparty_dependency
        spdlog
)
target_link_libraries(${TARGET_NAME} PUBLIC ${3rdparty_dependency})

# Set export symbols for the library
target_compile_definitions(${TARGET_NAME} PRIVATE IGE_EXPORT)
target_compile_definitions(${TARGET_NAME} PRIVATE IGE_BUILD_DLL)
# set platform symbol
if (WIN32)
    target_compile_definitions(${TARGET_NAME} PRIVATE IGE_PLATFORM_WINDOWS)
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
