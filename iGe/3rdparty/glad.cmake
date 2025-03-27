set(glad_SOURCE_DIR_ ${CMAKE_CURRENT_SOURCE_DIR}/glad)

file(GLOB glad_sources CONFIGURE_DEPENDS "${glad_SOURCE_DIR_}/src/*.c")

add_library(glad STATIC ${glad_sources})
target_include_directories(glad PUBLIC $<BUILD_INTERFACE:${glad_SOURCE_DIR_}/include>)
