set(xatlas_SOURCE_DIR_ ${CMAKE_CURRENT_SOURCE_DIR}/xatlas)

file(GLOB xatlas_sources CONFIGURE_DEPENDS "${xatlas_SOURCE_DIR_}/source/xatlas/*.cpp")

add_library(xatlas STATIC ${xatlas_sources})
target_include_directories(xatlas PUBLIC $<BUILD_INTERFACE:${xatlas_SOURCE_DIR_}/source/xatlas/>)
