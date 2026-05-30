cmake_minimum_required(VERSION 3.15)
project(uwpHelperDeps NONE)

set(UWP_X64_BASE "${CMAKE_CURRENT_LIST_DIR}/x64")

add_library(LIBUWP SHARED IMPORTED GLOBAL)

set_target_properties(LIBUWP PROPERTIES
    IMPORTED_LOCATION "${UWP_X64_BASE}/bin/libuwp.dll"
    IMPORTED_IMPLIB   "${UWP_X64_BASE}/lib/libuwp.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${UWP_X64_BASE}/include"
)
