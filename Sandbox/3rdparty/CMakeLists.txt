set(third_party_folder "ThirdParty")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

if (NOT TARGET assimp)
    option(ASSIMP_BUILD_TESTS "" OFF)
    option(ASSIMP_INSTALL "" OFF)
    option(ASSIMP_INSTALL_PDB "" OFF)
    add_subdirectory(assimp)
    set_target_properties(assimp PROPERTIES FOLDER ${third_party_folder}/assimp)
endif ()

if (NOT TARGET eigen)
    option(EIGEN_BUILD_DOC "" OFF)
    option(BUILD_TESTING "" OFF)
    option(ASSIMP_INSTALL_PDB "" OFF)
    add_subdirectory(eigen)
    set_target_properties(assimp PROPERTIES FOLDER ${third_party_folder}/assimp)
endif ()
