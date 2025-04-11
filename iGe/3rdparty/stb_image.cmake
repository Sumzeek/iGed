set(stb_image_SOURCE_DIR_ ${CMAKE_CURRENT_SOURCE_DIR}/stb_image)

file(GLOB stb_image_sources CONFIGURE_DEPENDS "${stb_image_SOURCE_DIR_}/*.cpp")

add_library(stb_image STATIC ${stb_image_sources})
target_include_directories(stb_image PUBLIC $<BUILD_INTERFACE:${stb_image_SOURCE_DIR_}>)
