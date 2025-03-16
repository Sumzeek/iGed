set(third_party_folder "ThirdParty")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if (NOT TARGET spdlog)
    option(SPDLOG_BUILD_EXAMPLE "" OFF)
    option(SPDLOG_INSTALL "" OFF)
    add_subdirectory(spdlog-1.x)
    set_target_properties(spdlog PROPERTIES FOLDER ${third_party_folder}/spdlog)
endif ()