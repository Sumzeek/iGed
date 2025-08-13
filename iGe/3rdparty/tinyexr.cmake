set(tinyexr_SOURCE_DIR_ ${CMAKE_CURRENT_SOURCE_DIR}/tinyexr)

file(GLOB tinyexr_sources CONFIGURE_DEPENDS "${tinyexr_SOURCE_DIR_}/*.c" "${tinyexr_SOURCE_DIR_}/*.cpp")

add_library(tinyexr STATIC ${tinyexr_sources})
target_compile_definitions(tinyexr PRIVATE TINYEXR_USE_MINIZ)
target_include_directories(tinyexr PUBLIC $<BUILD_INTERFACE:${tinyexr_SOURCE_DIR_}>)
