# Set source files (all .cpp files in the src directory)
file(GLOB SOURCES "src/iGed/*.cpp")

# Create a shared library (DLL)
add_library(iGed SHARED ${SOURCES})

# Set export symbols for the library
target_compile_definitions(iGed PRIVATE IGED_EXPORT)
target_compile_definitions(iGed PRIVATE IGED_BUILD_DLL)
# set platform symbol
if (WIN32)
    target_compile_definitions(iGed PRIVATE IGED_PLATFORM_WINDOWS)
else ()
    message(FATAL_ERROR "Unknown Platform: iGed only supports Windows!")
endif ()

# Set include directories
target_include_directories(iGed PUBLIC ${CMAKE_CURRENT_SOURCE_DIR}/include)

# Set output directories for the library
set_target_properties(iGed PROPERTIES
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"  # DLL files
        LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/bin"  # Shared library files
        ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"  # Static library files
)
