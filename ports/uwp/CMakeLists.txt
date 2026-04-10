cmake_minimum_required(VERSION 3.14)

set(CMAKE_SYSTEM_NAME WindowsStore CACHE STRING "Target UWP/Windows Store")
set(CMAKE_SYSTEM_VERSION 10.0.19041.0 CACHE STRING "Windows SDK version")
set(CMAKE_C_STANDARD 99)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

project(sm64coopdx-uwp LANGUAGES C CXX)

set_property(DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}" PROPERTY VS_STARTUP_PROJECT ${PROJECT_NAME})

get_filename_component(SM64COOPDX_REPO_ROOT "${CMAKE_CURRENT_LIST_DIR}/../.." ABSOLUTE)
find_program(SM64COOPDX_BASH_EXECUTABLE NAMES bash bash.exe PATHS "C:/msys64/usr/bin" NO_DEFAULT_PATH)
if(NOT SM64COOPDX_BASH_EXECUTABLE)
    find_program(SM64COOPDX_BASH_EXECUTABLE NAMES bash bash.exe)
endif()

function(sm64coopdx_pick_existing_tree out_var marker_path)
    set(_picked "")
    foreach(_candidate IN LISTS ARGN)
        if(_candidate AND EXISTS "${_candidate}/${marker_path}")
            set(_picked "${_candidate}")
            break()
        endif()
    endforeach()
    set(${out_var} "${_picked}" PARENT_SCOPE)
endfunction()

sm64coopdx_pick_existing_tree(SM64COOPDX_ROOT_DEFAULT "src/pc/pc_main.c"
    "${SM64COOPDX_REPO_ROOT}"
    "${CMAKE_CURRENT_LIST_DIR}/third_party/sm64coopdx"
    "${CMAKE_CURRENT_LIST_DIR}/deps/sm64coopdx"
)
sm64coopdx_pick_existing_tree(SM64COOPDX_UWP_DEPS_ROOT_DEFAULT "x64/lib/SDL2.lib"
    "${CMAKE_CURRENT_LIST_DIR}/third_party/uwp-deps"
)
sm64coopdx_pick_existing_tree(SM64COOPDX_GLAD_ROOT_DEFAULT "uwp/glad.c"
    "${CMAKE_CURRENT_LIST_DIR}/third_party/glad"
)
sm64coopdx_pick_existing_tree(SM64COOPDX_MESA_DLL_FALLBACK_DIR_DEFAULT "libglapi.dll"
    "${CMAKE_CURRENT_LIST_DIR}/third_party/uwp-deps/x64/bin"
)
sm64coopdx_pick_existing_tree(SM64COOPDX_VENDORED_DEPS_ROOT_DEFAULT "include/lua.h"
    "${CMAKE_CURRENT_LIST_DIR}/third_party/vcpkg-deps"
)

set(SM64COOPDX_ROOT "${SM64COOPDX_ROOT_DEFAULT}" CACHE PATH "sm64coopdx source tree with generated build/us_pc assets")
set(SM64COOPDX_UWP_DEPS_ROOT "${SM64COOPDX_UWP_DEPS_ROOT_DEFAULT}" CACHE PATH "Root of the UWP runtime bundle")
set(SM64COOPDX_GLAD_ROOT "${SM64COOPDX_GLAD_ROOT_DEFAULT}" CACHE PATH "Root of the GLAD source and headers used by the UWP build")
set(SM64COOPDX_MESA_DLL_FALLBACK_DIR "${SM64COOPDX_MESA_DLL_FALLBACK_DIR_DEFAULT}" CACHE PATH "Fallback Mesa DLL directory")
set(SM64COOPDX_VENDORED_DEPS_ROOT "${SM64COOPDX_VENDORED_DEPS_ROOT_DEFAULT}" CACHE PATH "Root of the Lua, zlib, and libjuice libraries used by the UWP build")
set(SM64COOPNET_SOURCE_ROOT "${CMAKE_CURRENT_LIST_DIR}/third_party/coopnet" CACHE PATH "CoopNet source checkout")
option(SM64COOPDX_USE_PLACEHOLDER_APP "Build the UWP dependency smoke-test app before linking sm64coopdx" ON)

set(UWP_DEP_X64 "${SM64COOPDX_UWP_DEPS_ROOT}/x64")
set(UWP_DEP_BIN "${UWP_DEP_X64}/bin")
set(UWP_DEP_LIB "${UWP_DEP_X64}/lib")
set(UWP_DEP_INCLUDE "${UWP_DEP_X64}/include")
set(SM64COOPDX_VENDORED_INCLUDE "${SM64COOPDX_VENDORED_DEPS_ROOT}/include")
set(SM64COOPDX_VENDORED_LIB "${SM64COOPDX_VENDORED_DEPS_ROOT}/lib")
set(SM64COOPDX_VENDORED_DEBUG_LIB "${SM64COOPDX_VENDORED_DEPS_ROOT}/debug/lib")

if(NOT SM64COOPDX_ROOT OR NOT EXISTS "${SM64COOPDX_ROOT}")
    message(FATAL_ERROR
        "SM64COOPDX_ROOT was not found. Prefer the repo root '${SM64COOPDX_REPO_ROOT}' or an in-repo layout such as '${CMAKE_CURRENT_LIST_DIR}/third_party/sm64coopdx' "
        "or point SM64COOPDX_ROOT at the game source tree.")
endif()

if(NOT EXISTS "${SM64COOPDX_ROOT}/tools/Makefile.assets")
    message(FATAL_ERROR "Missing tools/Makefile.assets under ${SM64COOPDX_ROOT}")
endif()

if(NOT EXISTS "${SM64COOPDX_ROOT}/tools/generate_level_headers.py")
    message(FATAL_ERROR "Missing tools/generate_level_headers.py under ${SM64COOPDX_ROOT}")
endif()

if(NOT SM64COOPDX_BASH_EXECUTABLE)
    message(FATAL_ERROR "bash.exe was not found. Install MSYS2 or set it on PATH so tools/Makefile.assets can run.")
endif()

# I run the asset Makefile during configure so the generated build/us_pc sources
# already exist by the time target_sources() gets evaluated.
message(STATUS "Generating build/us_pc assets via tools/Makefile.assets")
execute_process(
    COMMAND "${SM64COOPDX_BASH_EXECUTABLE}" -lc "cd '${SM64COOPDX_ROOT}/tools' && make -f Makefile.assets"
    WORKING_DIRECTORY "${SM64COOPDX_ROOT}"
    RESULT_VARIABLE SM64COOPDX_ASSET_GEN_RESULT
)

if(NOT SM64COOPDX_ASSET_GEN_RESULT EQUAL 0)
    message(FATAL_ERROR "tools/Makefile.assets failed with exit code ${SM64COOPDX_ASSET_GEN_RESULT}")
endif()

if(NOT SM64COOPDX_UWP_DEPS_ROOT OR NOT EXISTS "${SM64COOPDX_UWP_DEPS_ROOT}")
    message(FATAL_ERROR
        "SM64COOPDX_UWP_DEPS_ROOT was not found. Put the UWP runtime files under '${CMAKE_CURRENT_LIST_DIR}/third_party/uwp-deps'.")
endif()

if(NOT SM64COOPDX_GLAD_ROOT OR NOT EXISTS "${SM64COOPDX_GLAD_ROOT}")
    message(FATAL_ERROR
        "SM64COOPDX_GLAD_ROOT was not found. Put the GLAD files under '${CMAKE_CURRENT_LIST_DIR}/third_party/glad'.")
endif()

if(NOT SM64COOPDX_VENDORED_DEPS_ROOT OR NOT EXISTS "${SM64COOPDX_VENDORED_DEPS_ROOT}")
    message(FATAL_ERROR
        "SM64COOPDX_VENDORED_DEPS_ROOT was not found. Put the Lua, zlib, and libjuice files under '${CMAKE_CURRENT_LIST_DIR}/third_party/vcpkg-deps'.")
endif()

if(NOT EXISTS "${UWP_DEP_LIB}/SDL2.lib")
    message(FATAL_ERROR "SDL2.lib was not found under ${UWP_DEP_LIB}")
endif()

foreach(_vendored_header IN ITEMS
    "${SM64COOPDX_VENDORED_INCLUDE}/lua.h"
    "${SM64COOPDX_VENDORED_INCLUDE}/zlib.h"
    "${SM64COOPDX_VENDORED_INCLUDE}/juice/juice.h"
)
    if(NOT EXISTS "${_vendored_header}")
        message(FATAL_ERROR "Missing dependency header: ${_vendored_header}")
    endif()
endforeach()

foreach(_vendored_lib IN ITEMS
    "${SM64COOPDX_VENDORED_LIB}/lua.lib"
    "${SM64COOPDX_VENDORED_LIB}/zlib.lib"
    "${SM64COOPDX_VENDORED_LIB}/juice.lib"
    "${SM64COOPDX_VENDORED_DEBUG_LIB}/lua.lib"
    "${SM64COOPDX_VENDORED_DEBUG_LIB}/zlibd.lib"
    "${SM64COOPDX_VENDORED_DEBUG_LIB}/juice.lib"
)
    if(NOT EXISTS "${_vendored_lib}")
        message(FATAL_ERROR "Missing dependency library: ${_vendored_lib}")
    endif()
endforeach()

add_executable(${PROJECT_NAME} WIN32
    uwp/main.cpp
)

# I list the generated .c files explicitly here because CMake wants to know
# about real compilation units up front, not after the build has already begun.
set(SM64COOPDX_GENERATED_ASSET_SOURCES
    "${SM64COOPDX_ROOT}/build/us_pc/assets/mario_anim_data.c"
    "${SM64COOPDX_ROOT}/build/us_pc/assets/demo_data.c"
)

foreach(_generated_asset IN LISTS SM64COOPDX_GENERATED_ASSET_SOURCES)
    if(NOT EXISTS "${_generated_asset}")
        message(FATAL_ERROR "Expected generated asset source was not created: ${_generated_asset}")
    endif()
endforeach()

target_include_directories(${PROJECT_NAME} PRIVATE
    "${CMAKE_CURRENT_LIST_DIR}/uwp/compat"
    "${UWP_DEP_INCLUDE}"
    "${UWP_DEP_INCLUDE}/SDL2"
    "${SM64COOPDX_GLAD_ROOT}/deps/include"
    "${SM64COOPDX_ROOT}"
    "${SM64COOPDX_ROOT}/include"
    "${SM64COOPDX_ROOT}/sound"
    "${SM64COOPDX_ROOT}/src"
    "${SM64COOPDX_ROOT}/src/pc"
    "${SM64COOPDX_ROOT}/lib/coopnet/include"
    "${SM64COOPNET_SOURCE_ROOT}/common"
    "${SM64COOPDX_VENDORED_INCLUDE}"
    "${SM64COOPDX_ROOT}/build/us_pc"
    "${SM64COOPDX_ROOT}/build/us_pc/include"
)

target_compile_definitions(${PROJECT_NAME} PRIVATE
    _XBOX_UWP
    UWP_BUILD=1
    _LANGUAGE_C=1
    NON_MATCHING=1
    AVOID_UB=1
    VERSION_US=1
    ENHANCE_LEVEL_TEXTURES=1
    F3DEX_GBI_2E=1
    F3DEX_GBI_SHARED=1
    WINDOWS_BUILD=1
    WINSOCK=1
    HAVE_SDL2=1
    WAPI_SDL2=1
    RAPI_GL=1
    AAPI_SDL2=1
    CAPI_SDL2=1
    COOPNET=1
    LOGGING=1
    NOMINMAX
    SDL_MAIN_HANDLED=1
    _CRT_SECURE_NO_WARNINGS=1
    _CRT_NONSTDC_NO_WARNINGS=1
)

if(MSVC)
    target_compile_options(${PROJECT_NAME} PRIVATE
        "/FI${CMAKE_CURRENT_LIST_DIR}/uwp/compat/uwp_prefix.h"
        "/Zc:preprocessor"
    )
endif()

if(SM64COOPDX_USE_PLACEHOLDER_APP)
    target_compile_definitions(${PROJECT_NAME} PRIVATE SM64COOPDX_USE_PLACEHOLDER_APP=1)
else()
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
    list(FILTER SM64COOPDX_GAME_SOURCES EXCLUDE REGEX "/src/pc/gfx/gfx_direct3d11\\.cpp$")
    list(FILTER SM64COOPDX_GAME_SOURCES EXCLUDE REGEX "/src/pc/gfx/gfx_dxgi\\.cpp$")
    list(FILTER SM64COOPDX_GAME_SOURCES EXCLUDE REGEX "/src/pc/gfx/gfx_opengl_legacy\\.c$")
    list(FILTER SM64COOPDX_GAME_SOURCES EXCLUDE REGEX "/src/game/main\\.c$")
    list(FILTER SM64COOPDX_GAME_SOURCES EXCLUDE REGEX "/lib/src/leointerrupt\\.c$")
    list(FILTER SM64COOPDX_GAME_SOURCES EXCLUDE REGEX "/lib/src/unk_shindou_file_3\\.c$")
    list(FILTER SM64COOPDX_GAME_SOURCES EXCLUDE REGEX "\\.inc\\.c$")
    list(REMOVE_ITEM SM64COOPDX_GAME_SOURCES "${SM64COOPDX_ROOT}/src/pc/pc_main.c")

    target_sources(${PROJECT_NAME} PRIVATE
        uwp/sm64coopdx_entry.cpp
        uwp/compat/dirent.cpp
        uwp/compat/pthread.cpp
        uwp/compat/update_checker_stub.c
        uwp/compat/bzero.c
        "${SM64COOPDX_GLAD_ROOT}/uwp/glad.c"
        "${SM64COOPDX_ROOT}/src/pc/pc_main.c"
        ${SM64COOPDX_GENERATED_ASSET_SOURCES}
        ${SM64COOPDX_GAME_SOURCES}
    )

    file(GLOB SM64COOPNET_COMMON_SOURCES CONFIGURE_DEPENDS
        "${SM64COOPNET_SOURCE_ROOT}/common/*.cpp"
    )
    target_sources(${PROJECT_NAME} PRIVATE ${SM64COOPNET_COMMON_SOURCES})

    set_source_files_properties("${SM64COOPDX_ROOT}/src/pc/pc_main.c" PROPERTIES
        COMPILE_OPTIONS "/Dmain=sm64coopdx_main"
        LANGUAGE C
    )
    set_source_files_properties("${SM64COOPDX_ROOT}/src/pc/mods/mod_fs.cpp" PROPERTIES
        COMPILE_OPTIONS "/FI${CMAKE_CURRENT_LIST_DIR}/uwp/compat/strings.h"
    )
endif()

target_link_libraries(${PROJECT_NAME} PRIVATE
    "${UWP_DEP_LIB}/SDL2.lib"
    "$<$<CONFIG:Debug>:${SM64COOPDX_VENDORED_DEBUG_LIB}/lua.lib>"
    "$<$<NOT:$<CONFIG:Debug>>:${SM64COOPDX_VENDORED_LIB}/lua.lib>"
    "$<$<CONFIG:Debug>:${SM64COOPDX_VENDORED_DEBUG_LIB}/zlibd.lib>"
    "$<$<NOT:$<CONFIG:Debug>>:${SM64COOPDX_VENDORED_LIB}/zlib.lib>"
    "$<$<CONFIG:Debug>:${SM64COOPDX_VENDORED_DEBUG_LIB}/juice.lib>"
    "$<$<NOT:$<CONFIG:Debug>>:${SM64COOPDX_VENDORED_LIB}/juice.lib>"
    opengl32.lib
    Ws2_32.lib
    bcrypt.lib
    iphlpapi.lib
    WindowsApp.lib
    OneCore.lib
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VS_WINDOWS_TARGET_PLATFORM_MIN_VERSION 10.0.19041.0
)

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

foreach(_runtime IN ITEMS
    SDL2.dll
    opengl32.dll
    libgallium_wgl.dll
    z-1.dll
    dxil.dll
)
    sm64coopdx_deploy_file("${UWP_DEP_BIN}/${_runtime}" ".")
endforeach()

if(EXISTS "${UWP_DEP_BIN}/libglapi.dll")
    sm64coopdx_deploy_file("${UWP_DEP_BIN}/libglapi.dll" ".")
else()
    sm64coopdx_deploy_file("${SM64COOPDX_MESA_DLL_FALLBACK_DIR}/libglapi.dll" ".")
endif()

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
