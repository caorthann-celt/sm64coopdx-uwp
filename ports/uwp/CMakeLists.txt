cmake_minimum_required(VERSION 3.14)

set(CMAKE_SYSTEM_NAME WindowsStore CACHE STRING "Target UWP/Windows Store")
set(CMAKE_SYSTEM_VERSION 10.0.19041.0 CACHE STRING "Windows SDK version")
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(sm64coopdx-uwp LANGUAGES C CXX)

if(CMAKE_CONFIGURATION_TYPES)
    set(CMAKE_CONFIGURATION_TYPES "Debug;Release" CACHE STRING "Supported build types" FORCE)
endif()

set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

get_filename_component(SM64COOPDX_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)

option(SM64COOPDX_BUILD_GAME_DLL "Build sm64coopdx game code as a UWP DLL" OFF)
option(SM64COOPDX_PACKAGE_ONLY "Build only the UWP packaging wrapper" OFF)

set(SM64COOPDX_ROOT "${SM64COOPDX_REPO_ROOT}" CACHE PATH "sm64coopdx source tree")
set(SM64COOPDX_UWP_DEPS_ROOT "${CMAKE_CURRENT_LIST_DIR}/third_party/uwp-deps" CACHE PATH "Root of the UWP runtime bundle")
set(SM64COOPDX_VENDORED_DEPS_ROOT "${CMAKE_CURRENT_LIST_DIR}/third_party/vcpkg-deps" CACHE PATH "Root of the zlib and libjuice libraries used by the UWP build")
set(SM64COOPDX_LUA53_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/third_party/lua-5.3.5" CACHE PATH "Lua 5.3 source used by the UWP build")
set(SM64COOPNET_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/third_party/coopnet" CACHE PATH "CoopNet source checkout")
set(SM64COOPDX_GAME_DLL_DIR "" CACHE PATH "Directory containing sm64coopdx_game.dll and sm64coopdx_game.lib")
set(SM64COOPDX_GAME_DLL "" CACHE FILEPATH "Optional explicit path to sm64coopdx_game.dll")
set(SM64COOPDX_GAME_IMPORT_LIB "" CACHE FILEPATH "Optional explicit path to sm64coopdx_game.lib")

set(UWP_DEP_X64 "${SM64COOPDX_UWP_DEPS_ROOT}/x64")
set(UWP_DEP_BIN "${UWP_DEP_X64}/bin")
set(UWP_DEP_LIB "${UWP_DEP_X64}/lib")
set(UWP_DEP_INCLUDE "${UWP_DEP_X64}/include")
set(SM64COOPDX_VENDORED_INCLUDE "${SM64COOPDX_VENDORED_DEPS_ROOT}/include")
set(SM64COOPDX_VENDORED_LIB "${SM64COOPDX_VENDORED_DEPS_ROOT}/lib")
set(SM64COOPDX_VENDORED_DEBUG_LIB "${SM64COOPDX_VENDORED_DEPS_ROOT}/debug/lib")
set(SM64COOPDX_MSVC_UWP_CRT_LIB_DIR "" CACHE PATH "Optional MSVC UWP/store CRT lib directory for clang-cl Ninja builds")
if(NOT SM64COOPDX_MSVC_UWP_CRT_LIB_DIR)
    if(DEFINED ENV{VCToolsInstallDir} AND EXISTS "$ENV{VCToolsInstallDir}/lib/x64/store/vcruntime.lib")
        set(SM64COOPDX_MSVC_UWP_CRT_LIB_DIR "$ENV{VCToolsInstallDir}/lib/x64/store" CACHE PATH "Optional MSVC UWP/store CRT lib directory for clang-cl Ninja builds" FORCE)
    else()
        file(GLOB _msvc_store_crt_dirs
            "C:/Program Files/Microsoft Visual Studio/2022/*/VC/Tools/MSVC/*/lib/x64/store"
            "C:/Program Files (x86)/Microsoft Visual Studio/2022/*/VC/Tools/MSVC/*/lib/x64/store"
        )
        list(SORT _msvc_store_crt_dirs ORDER DESCENDING)
        foreach(_msvc_store_crt_dir IN LISTS _msvc_store_crt_dirs)
            if(EXISTS "${_msvc_store_crt_dir}/vcruntime.lib")
                set(SM64COOPDX_MSVC_UWP_CRT_LIB_DIR "${_msvc_store_crt_dir}" CACHE PATH "Optional MSVC UWP/store CRT lib directory for clang-cl Ninja builds" FORCE)
                break()
            endif()
        endforeach()
    endif()
endif()

add_library(sm64coopdx_sdl2 SHARED IMPORTED GLOBAL)
set_target_properties(sm64coopdx_sdl2 PROPERTIES
    IMPORTED_LOCATION "${UWP_DEP_BIN}/SDL2.dll"
    IMPORTED_IMPLIB "${UWP_DEP_LIB}/SDL2.lib"
    INTERFACE_INCLUDE_DIRECTORIES "${UWP_DEP_INCLUDE};${UWP_DEP_INCLUDE}/SDL2"
)
set(SM64COOPDX_SDL2_TARGET sm64coopdx_sdl2)

set(SM64COOPDX_COMMON_DEFINES
    _XBOX_UWP
    UWP_BUILD=1
    __WINRT__
    __PRFCHWINTRIN_H
    WIN32_LEAN_AND_MEAN
    NOMINMAX
    SDL_MAIN_HANDLED=1
    _CRT_SECURE_NO_WARNINGS=1
    _CRT_NONSTDC_NO_WARNINGS=1
)

function(sm64coopdx_require_path path message_text)
    if(NOT path OR NOT EXISTS "${path}")
        message(FATAL_ERROR "${message_text}: ${path}")
    endif()
endfunction()

function(sm64coopdx_apply_uwp_compile_options target_name)
    if(MSVC AND NOT CMAKE_C_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target_name} PRIVATE
            "/FI${CMAKE_CURRENT_LIST_DIR}/uwp/compat/uwp_prefix.h"
            "/Zc:preprocessor"
        )
    elseif(CMAKE_C_COMPILER_ID MATCHES "Clang")
        target_compile_options(${target_name} PRIVATE
            "/FI${CMAKE_CURRENT_LIST_DIR}/uwp/compat/uwp_prefix.h"
        )
    endif()
endfunction()

function(sm64coopdx_deploy_file file_path deploy_location)
    if(NOT EXISTS "${file_path}")
        message(WARNING "Skipping missing deployment file: ${file_path}")
        return()
    endif()

    target_sources(${PROJECT_NAME} PRIVATE "${file_path}")
    set_source_files_properties("${file_path}" PROPERTIES
        VS_COPY_TO_OUT_DIR Always
        VS_DEPLOYMENT_CONTENT 1
        VS_DEPLOYMENT_LOCATION "${deploy_location}"
    )
endfunction()

function(sm64coopdx_deploy_tree source_root deploy_root)
    if(NOT EXISTS "${source_root}")
        message(WARNING "Skipping missing deployment tree: ${source_root}")
        return()
    endif()

    file(GLOB_RECURSE _tree_files CONFIGURE_DEPENDS "${source_root}/*")
    foreach(_tree_file IN LISTS _tree_files)
        if(IS_DIRECTORY "${_tree_file}")
            continue()
        endif()

        file(RELATIVE_PATH _relative_path "${source_root}" "${_tree_file}")
        get_filename_component(_relative_dir "${_relative_path}" DIRECTORY)

        if(_relative_dir STREQUAL "")
            set(_deploy_location "${deploy_root}")
        else()
            string(REPLACE "\\" "/" _relative_dir "${_relative_dir}")
            set(_deploy_location "${deploy_root}/${_relative_dir}")
        endif()

        sm64coopdx_deploy_file("${_tree_file}" "${_deploy_location}")
    endforeach()
endfunction()

function(sm64coopdx_get_imported_runtime target_name output_var)
    get_target_property(_runtime "${target_name}" IMPORTED_LOCATION_RELEASE)
    if(NOT _runtime)
        get_target_property(_runtime "${target_name}" IMPORTED_LOCATION_DEBUG)
    endif()
    if(NOT _runtime)
        get_target_property(_runtime "${target_name}" IMPORTED_LOCATION_NOCONFIG)
    endif()
    if(NOT _runtime)
        get_target_property(_runtime "${target_name}" IMPORTED_LOCATION)
    endif()
    set("${output_var}" "${_runtime}" PARENT_SCOPE)
endfunction()

function(sm64coopdx_stage_common_package_content)
    sm64coopdx_get_imported_runtime("${SM64COOPDX_SDL2_TARGET}" SM64COOPDX_SDL2_RUNTIME)
    if(SM64COOPDX_SDL2_RUNTIME)
        sm64coopdx_deploy_file("${SM64COOPDX_SDL2_RUNTIME}" ".")
    endif()

    foreach(_runtime IN ITEMS
        d3dcompiler_47.dll
        dxil.dll
        libgallium_wgl.dll
        libuwp.dll
        opengl32.dll
        z-1.dll
    )
        sm64coopdx_deploy_file("${UWP_DEP_BIN}/${_runtime}" ".")
    endforeach()

    sm64coopdx_deploy_file("${CMAKE_CURRENT_LIST_DIR}/uwp/Package.appxmanifest" ".")

    file(GLOB SM64COOPDX_UWP_ASSET_FILES CONFIGURE_DEPENDS
        "${CMAKE_CURRENT_LIST_DIR}/uwp/Assets/*.png"
    )
    foreach(_asset_file IN LISTS SM64COOPDX_UWP_ASSET_FILES)
        sm64coopdx_deploy_file("${_asset_file}" "Assets")
    endforeach()

    sm64coopdx_deploy_tree("${SM64COOPDX_ROOT}/dynos" "dynos")
    sm64coopdx_deploy_tree("${SM64COOPDX_ROOT}/mods" "mods")
    sm64coopdx_deploy_tree("${SM64COOPDX_ROOT}/palettes" "palettes")
    sm64coopdx_deploy_tree("${SM64COOPDX_ROOT}/lang" "lang")
endfunction()

sm64coopdx_require_path("${SM64COOPDX_ROOT}" "SM64COOPDX_ROOT was not found")
sm64coopdx_require_path("${SM64COOPDX_UWP_DEPS_ROOT}" "SM64COOPDX_UWP_DEPS_ROOT was not found")
sm64coopdx_require_path("${SM64COOPDX_VENDORED_DEPS_ROOT}" "SM64COOPDX_VENDORED_DEPS_ROOT was not found")

foreach(_vendored_header IN ITEMS
    "${SM64COOPDX_VENDORED_INCLUDE}/zlib.h"
    "${SM64COOPDX_VENDORED_INCLUDE}/juice/juice.h"
)
    sm64coopdx_require_path("${_vendored_header}" "Missing dependency header")
endforeach()

foreach(_lua_header IN ITEMS
    "${SM64COOPDX_ROOT}/lib/lua/include/lua.h"
    "${SM64COOPDX_ROOT}/lib/lua/include/lualib.h"
    "${SM64COOPDX_ROOT}/lib/lua/include/lauxlib.h"
)
    sm64coopdx_require_path("${_lua_header}" "Missing upstream Lua header")
endforeach()

sm64coopdx_require_path("${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lapi.c" "Missing Lua 5.3 source")

foreach(_vendored_lib IN ITEMS
    "${SM64COOPDX_VENDORED_LIB}/zlib.lib"
    "${SM64COOPDX_VENDORED_LIB}/juice.lib"
    "${SM64COOPDX_VENDORED_DEBUG_LIB}/zlibd.lib"
    "${SM64COOPDX_VENDORED_DEBUG_LIB}/juice.lib"
)
    sm64coopdx_require_path("${_vendored_lib}" "Missing dependency library")
endforeach()

if(SM64COOPDX_PACKAGE_ONLY)
    set(SM64COOPDX_RESOLVED_GAME_DLL_DIR "${SM64COOPDX_GAME_DLL_DIR}")
    if(SM64COOPDX_RESOLVED_GAME_DLL_DIR AND NOT IS_ABSOLUTE "${SM64COOPDX_RESOLVED_GAME_DLL_DIR}")
        get_filename_component(SM64COOPDX_RESOLVED_GAME_DLL_DIR "${SM64COOPDX_REPO_ROOT}/${SM64COOPDX_RESOLVED_GAME_DLL_DIR}" ABSOLUTE)
    endif()

    set(SM64COOPDX_RESOLVED_GAME_DLL "${SM64COOPDX_GAME_DLL}")
    if(NOT SM64COOPDX_RESOLVED_GAME_DLL AND SM64COOPDX_RESOLVED_GAME_DLL_DIR)
        set(SM64COOPDX_RESOLVED_GAME_DLL "${SM64COOPDX_RESOLVED_GAME_DLL_DIR}/sm64coopdx_game.dll")
    elseif(SM64COOPDX_RESOLVED_GAME_DLL AND NOT IS_ABSOLUTE "${SM64COOPDX_RESOLVED_GAME_DLL}")
        get_filename_component(SM64COOPDX_RESOLVED_GAME_DLL "${SM64COOPDX_REPO_ROOT}/${SM64COOPDX_RESOLVED_GAME_DLL}" ABSOLUTE)
    endif()

    set(SM64COOPDX_RESOLVED_GAME_IMPORT_LIB "${SM64COOPDX_GAME_IMPORT_LIB}")
    if(NOT SM64COOPDX_RESOLVED_GAME_IMPORT_LIB AND SM64COOPDX_RESOLVED_GAME_DLL_DIR)
        set(SM64COOPDX_RESOLVED_GAME_IMPORT_LIB "${SM64COOPDX_RESOLVED_GAME_DLL_DIR}/sm64coopdx_game.lib")
    elseif(SM64COOPDX_RESOLVED_GAME_IMPORT_LIB AND NOT IS_ABSOLUTE "${SM64COOPDX_RESOLVED_GAME_IMPORT_LIB}")
        get_filename_component(SM64COOPDX_RESOLVED_GAME_IMPORT_LIB "${SM64COOPDX_REPO_ROOT}/${SM64COOPDX_RESOLVED_GAME_IMPORT_LIB}" ABSOLUTE)
    endif()

    sm64coopdx_require_path("${SM64COOPDX_RESOLVED_GAME_DLL}" "Set SM64COOPDX_GAME_DLL_DIR or SM64COOPDX_GAME_DLL to the UWP-built sm64coopdx_game.dll")
    sm64coopdx_require_path("${SM64COOPDX_RESOLVED_GAME_IMPORT_LIB}" "Set SM64COOPDX_GAME_IMPORT_LIB to the UWP-built sm64coopdx_game.lib")

    add_executable(${PROJECT_NAME} WIN32
        uwp/main.cpp
        uwp/sm64coopdx_entry.cpp
    )

    target_include_directories(${PROJECT_NAME} PRIVATE
        "${CMAKE_CURRENT_LIST_DIR}/uwp/compat"
        "${UWP_DEP_INCLUDE}"
    )
    target_compile_definitions(${PROJECT_NAME} PRIVATE ${SM64COOPDX_COMMON_DEFINES})

    target_link_libraries(${PROJECT_NAME} PRIVATE
        "${SM64COOPDX_RESOLVED_GAME_IMPORT_LIB}"
        ${SM64COOPDX_SDL2_TARGET}
        WindowsApp.lib
        OneCore.lib
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES
        VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION 10.0.19041.0
        VS_SDK_REFERENCES "Microsoft.VCLibs, Version=14.0"
    )

    sm64coopdx_deploy_file("${SM64COOPDX_RESOLVED_GAME_DLL}" ".")
    sm64coopdx_stage_common_package_content()
    return()
endif()

if(NOT SM64COOPDX_BUILD_GAME_DLL)
    message(FATAL_ERROR
        "Choose a build mode: configure with SM64COOPDX_BUILD_GAME_DLL=ON for the clang-built game DLL, "
        "or SM64COOPDX_PACKAGE_ONLY=ON for the Visual Studio UWP package wrapper.")
endif()

include("${CMAKE_CURRENT_LIST_DIR}/cmake/PrebuildAssets.cmake")

add_library(sm64coopdx_game SHARED)
set_target_properties(sm64coopdx_game PROPERTIES
    OUTPUT_NAME sm64coopdx_game
    WINDOWS_EXPORT_ALL_SYMBOLS TRUE
)

sm64coopdx_add_prebuild_assets(sm64coopdx_prebuild_assets SM64COOPDX_GENERATED_ASSET_SOURCES)

add_library(glew_static STATIC
    "${CMAKE_CURRENT_LIST_DIR}/third_party/glew/glew.c"
)
target_include_directories(glew_static PRIVATE
    "${UWP_DEP_INCLUDE}"
    "${UWP_DEP_INCLUDE}/SDL2"
    "${SM64COOPDX_ROOT}/include"
)
target_compile_definitions(glew_static PRIVATE
    ${SM64COOPDX_COMMON_DEFINES}
    GLEW_STATIC=1
)
sm64coopdx_apply_uwp_compile_options(glew_static)

add_library(lua53_uwp STATIC
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lapi.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lauxlib.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lbaselib.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lbitlib.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lcode.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lcorolib.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lctype.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/ldblib.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/ldebug.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/ldo.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/ldump.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lfunc.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lgc.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/linit.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/liolib.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/llex.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lmathlib.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lmem.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/loadlib.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lobject.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lopcodes.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/loslib.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lparser.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lstate.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lstring.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lstrlib.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/ltable.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/ltablib.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/ltm.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lundump.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lutf8lib.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lvm.c"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src/lzio.c"
)
target_include_directories(lua53_uwp PRIVATE
    "${SM64COOPDX_ROOT}/lib/lua/include"
    "${SM64COOPDX_LUA53_SOURCE_ROOT}/src"
)
target_compile_definitions(lua53_uwp PRIVATE
    ${SM64COOPDX_COMMON_DEFINES}
)

target_include_directories(sm64coopdx_game PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/uwp/compat"
    "${UWP_DEP_INCLUDE}"
    "${UWP_DEP_INCLUDE}/SDL2"
    "${SM64COOPDX_ROOT}"
    "${SM64COOPDX_ROOT}/include"
    "${SM64COOPDX_ROOT}/lib/lua/include"
    "${SM64COOPDX_ROOT}/sound"
    "${SM64COOPDX_ROOT}/src"
    "${SM64COOPDX_ROOT}/src/pc"
    "${SM64COOPDX_ROOT}/lib/coopnet/include"
    "${SM64COOPNET_SOURCE_ROOT}/common"
    "${SM64COOPDX_VENDORED_INCLUDE}"
    "${SM64COOPDX_ROOT}/build/us_pc"
    "${SM64COOPDX_ROOT}/build/us_pc/include"
)

target_compile_definitions(sm64coopdx_game PRIVATE
    ${SM64COOPDX_COMMON_DEFINES}
    _LANGUAGE_C=1
    NON_MATCHING=1
    AVOID_UB=1
    VERSION_US=1
    ENHANCE_LEVEL_TEXTURES=1
    F3DEX_GBI_2E=1
    F3DEX_GBI_SHARED=1
    WINDOWS_BUILD=1
    WINSOCK=1
    _AMD64_
    HAVE_SDL2=1
    WAPI_SDL2=1
    AAPI_SDL2=1
    CAPI_SDL2=1
    COOPNET=1
    LOGGING=1
    _USE_MATH_DEFINES
)

sm64coopdx_apply_uwp_compile_options(sm64coopdx_game)

set(SM64COOPDX_SOURCE_DIRS
    src
    src/engine
    src/game
    src/goddard
    src/goddard/dynlists
    src/audio
    src/menu
    src/buffers
    src/pc
    src/pc/audio
    src/pc/controller
    src/pc/dev
    src/pc/djui
    src/pc/fs
    src/pc/fs/packtypes
    src/pc/gfx
    src/pc/lua
    src/pc/lua/utils
    src/pc/mods
    src/pc/network
    src/pc/network/coopnet
    src/pc/network/packets
    src/pc/network/socket
    src/pc/os
    src/pc/utils
    src/pc/utils/miniz
    src/pc/mumble
    data
    bin
    actors
    lib/src
    sound
    assets
)

set(SM64COOPDX_GAME_SOURCES)
foreach(_source_dir IN LISTS SM64COOPDX_SOURCE_DIRS)
    file(GLOB _c_sources CONFIGURE_DEPENDS
        "${SM64COOPDX_ROOT}/${_source_dir}/*.c"
    )
    file(GLOB _cpp_sources CONFIGURE_DEPENDS
        "${SM64COOPDX_ROOT}/${_source_dir}/*.cpp"
    )
    list(APPEND SM64COOPDX_GAME_SOURCES ${_c_sources} ${_cpp_sources})
endforeach()

file(GLOB_RECURSE SM64COOPDX_LEVEL_SOURCES CONFIGURE_DEPENDS
    "${SM64COOPDX_ROOT}/levels/*.c"
)
list(APPEND SM64COOPDX_GAME_SOURCES ${SM64COOPDX_LEVEL_SOURCES})

list(FILTER SM64COOPDX_GAME_SOURCES EXCLUDE REGEX "/src/pc/discord/")
list(FILTER SM64COOPDX_GAME_SOURCES EXCLUDE REGEX "/src/pc/update_checker\\.c$")
list(FILTER SM64COOPDX_GAME_SOURCES EXCLUDE REGEX "/src/pc/audio/audio_sdl1\\.c$")
list(FILTER SM64COOPDX_GAME_SOURCES EXCLUDE REGEX "/src/pc/controller/controller_sdl1\\.c$")
list(FILTER SM64COOPDX_GAME_SOURCES EXCLUDE REGEX "/src/pc/gfx/gfx_sdl1\\.c$")
list(FILTER SM64COOPDX_GAME_SOURCES EXCLUDE REGEX "/src/pc/gfx/gfx_dxgi\\.cpp$")
list(FILTER SM64COOPDX_GAME_SOURCES EXCLUDE REGEX "/src/pc/gfx/gfx_opengl_legacy\\.c$")
list(FILTER SM64COOPDX_GAME_SOURCES EXCLUDE REGEX "/src/game/main\\.c$")
list(FILTER SM64COOPDX_GAME_SOURCES EXCLUDE REGEX "/lib/src/leointerrupt\\.c$")
list(FILTER SM64COOPDX_GAME_SOURCES EXCLUDE REGEX "/lib/src/unk_shindou_file_3\\.c$")
list(FILTER SM64COOPDX_GAME_SOURCES EXCLUDE REGEX "\\.inc\\.c$")
list(REMOVE_ITEM SM64COOPDX_GAME_SOURCES "${SM64COOPDX_ROOT}/src/pc/pc_main.c")

target_sources(sm64coopdx_game PRIVATE
    uwp/compat/dirent.cpp
    uwp/compat/pthread.cpp
    uwp/compat/sdl_winrt_stubs.cpp
    uwp/compat/uwp_display_size.cpp
    uwp/compat/uwp_input_pane.cpp
    uwp/compat/uwp_coopnet_identity.c
    uwp/compat/uwp_storage_root.c
    uwp/compat/update_checker_stub.c
    uwp/compat/bzero.c
    uwp/gfx/gfx_dxgi_uwp.cpp
    "${SM64COOPDX_ROOT}/src/pc/pc_main.c"
    ${SM64COOPDX_GENERATED_ASSET_SOURCES}
    ${SM64COOPDX_GAME_SOURCES}
)
add_dependencies(sm64coopdx_game sm64coopdx_prebuild_assets)

file(GLOB SM64COOPNET_COMMON_SOURCES CONFIGURE_DEPENDS
    "${SM64COOPNET_SOURCE_ROOT}/common/*.cpp"
)
target_sources(sm64coopdx_game PRIVATE ${SM64COOPNET_COMMON_SOURCES})

set_source_files_properties("${SM64COOPDX_ROOT}/src/pc/pc_main.c" PROPERTIES
    COMPILE_OPTIONS "/Dmain=sm64coopdx_main"
    LANGUAGE C
)
set_source_files_properties("${SM64COOPDX_ROOT}/src/pc/mods/mod_fs.cpp" PROPERTIES
    COMPILE_OPTIONS "/FI${CMAKE_CURRENT_LIST_DIR}/uwp/compat/strings.h"
)
set_source_files_properties("${SM64COOPDX_ROOT}/data/dynos_level.cpp" PROPERTIES
    COMPILE_OPTIONS "/FI${CMAKE_CURRENT_LIST_DIR}/uwp/compat/dynos_level_linkage.h"
)
set_source_files_properties("${SM64COOPDX_ROOT}/src/engine/math_util.c" PROPERTIES
    COMPILE_OPTIONS "/Dinline="
)

target_link_libraries(sm64coopdx_game PRIVATE
    ${SM64COOPDX_SDL2_TARGET}
    glew_static
    lua53_uwp
    "${UWP_DEP_LIB}/libuwp.lib"
    "$<$<CONFIG:Debug>:${SM64COOPDX_VENDORED_DEBUG_LIB}/zlibd.lib>"
    "$<$<NOT:$<CONFIG:Debug>>:${SM64COOPDX_VENDORED_LIB}/zlib.lib>"
    "$<$<CONFIG:Debug>:${SM64COOPDX_VENDORED_DEBUG_LIB}/juice.lib>"
    "$<$<NOT:$<CONFIG:Debug>>:${SM64COOPDX_VENDORED_LIB}/juice.lib>"
    opengl32.lib
    d3d11.lib
    dxgi.lib
    d3dcompiler.lib
    Ws2_32.lib
    bcrypt.lib
    iphlpapi.lib
    WindowsApp.lib
    OneCore.lib
)
if(SM64COOPDX_MSVC_UWP_CRT_LIB_DIR)
    target_link_options(sm64coopdx_game PRIVATE "/LIBPATH:${SM64COOPDX_MSVC_UWP_CRT_LIB_DIR}")
endif()
