set(third_party_folder "ThirdParty")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if (NOT TARGET assimp)
    option(ASSIMP_BUILD_TESTS "" OFF)
    option(ASSIMP_INSTALL "" OFF)
    option(ASSIMP_INSTALL_PDB "" OFF)
    add_subdirectory(assimp)
    set_target_properties(assimp PROPERTIES FOLDER ${third_party_folder}/assimp)
endif ()

if (NOT TARGET meshoptimizer)
    add_subdirectory(meshoptimizer)
    set_target_properties(meshoptimizer PROPERTIES FOLDER ${third_party_folder}/meshoptimizer)
endif ()

if (NOT TARGET vcglib)
    add_subdirectory(vcglib)
    set_target_properties(vcglib PROPERTIES FOLDER ${third_party_folder}/vcglib)
endif ()

#if (NOT TARGET xatlas)
#    include(xatlas.cmake)
#    set_target_properties(xatlas PROPERTIES FOLDER ${third_party_folder}/xatlas)
#endif ()

#if (NOT TARGET UVAtlas)
#    add_subdirectory(uvatlas)
#    set_target_properties(UVAtlas PROPERTIES FOLDER ${third_party_folder}/uvatlas)
#endif ()
