set(glm_SOURCE_DIR_ ${CMAKE_CURRENT_SOURCE_DIR}/glm)

file(GLOB glm_sources CONFIGURE_DEPENDS "${glm_SOURCE_DIR_}/glm/glm.cppm")

add_library(glm STATIC)
add_library(glm::glm ALIAS glm)

target_sources(glm PUBLIC
        FILE_SET cxx_modules TYPE CXX_MODULES FILES ${glm_sources}
)

target_include_directories(glm PUBLIC $<BUILD_INTERFACE:${glm_SOURCE_DIR_}>)
