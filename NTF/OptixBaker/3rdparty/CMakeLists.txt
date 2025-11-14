set(third_party_folder "ThirdParty")

set(BUILD_SHARED_LIBS OFF CACHE BOOL "" FORCE)

set(PYBIND11_FINDPYTHON NEW)
add_subdirectory(pybind11)
