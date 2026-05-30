function(sm64coopdx_prebuild_collect_texture_outputs out_outputs out_inputs out_targets)
    set(_texture_inputs)
    set(_texture_outputs)
    set(_texture_targets)
    set(_texture_roots actors textures levels bin assets)
    set(_texture_formats rgba32 rgba16 ia16 ia8 ia4 ia1 i8 i4)

    foreach(_texture_root IN LISTS _texture_roots)
        file(GLOB_RECURSE _pngs CONFIGURE_DEPENDS
            "${SM64COOPDX_ROOT}/${_texture_root}/*.png"
        )

        foreach(_png IN LISTS _pngs)
            get_filename_component(_png_name "${_png}" NAME)
            string(REPLACE "." ";" _png_parts "${_png_name}")
            list(LENGTH _png_parts _png_part_count)

            if(_png_part_count LESS 3)
                continue()
            endif()

            math(EXPR _format_index "${_png_part_count} - 2")
            list(GET _png_parts ${_format_index} _format)

            if(NOT _format IN_LIST _texture_formats)
                continue()
            endif()

            file(RELATIVE_PATH _relative_png "${SM64COOPDX_ROOT}" "${_png}")
            string(REGEX REPLACE "\\.png$" ".inc.c" _relative_target "${_relative_png}")

            list(APPEND _texture_inputs "${_png}")
            list(APPEND _texture_targets "build/us_pc/${_relative_target}")
            list(APPEND _texture_outputs "${SM64COOPDX_ROOT}/build/us_pc/${_relative_target}")
        endforeach()
    endforeach()

    set(${out_outputs} ${_texture_outputs} PARENT_SCOPE)
    set(${out_inputs} ${_texture_inputs} PARENT_SCOPE)
    set(${out_targets} ${_texture_targets} PARENT_SCOPE)
endfunction()

function(sm64coopdx_add_prebuild_assets target_name generated_sources_out)
    set(SM64COOPDX_BUILD_ROOT "${SM64COOPDX_ROOT}/build/us_pc")
    set(SM64COOPDX_MSYS2_ROOT "C:/msys64" CACHE PATH "MSYS2 root used to run upstream Makefile asset rules")
    set(SM64COOPDX_MSYS_BASH "${SM64COOPDX_MSYS2_ROOT}/usr/bin/bash.exe" CACHE FILEPATH "MSYS2 bash executable")

    if(NOT EXISTS "${SM64COOPDX_MSYS_BASH}")
        message(FATAL_ERROR "MSYS2 bash was not found: ${SM64COOPDX_MSYS_BASH}")
    endif()

    sm64coopdx_prebuild_collect_texture_outputs(
        SM64COOPDX_TEXTURE_OUTPUTS
        SM64COOPDX_TEXTURE_INPUTS
        SM64COOPDX_TEXTURE_TARGETS
    )

    set(_generated_c_sources
        "${SM64COOPDX_BUILD_ROOT}/assets/mario_anim_data.c"
        "${SM64COOPDX_BUILD_ROOT}/assets/demo_data.c"
    )

    set(_make_targets
        build/us_pc/assets/mario_anim_data.c
        build/us_pc/assets/demo_data.c
        build/us_pc/include/level_headers.h
        build/us_pc/text/us/define_courses.inc.c
        build/us_pc/text/us/define_text.inc.c
        build/us_pc/sound/sound_data.ctl.inc.c
        build/us_pc/sound/sound_data.tbl.inc.c
        build/us_pc/sound/sequences.bin.inc.c
        build/us_pc/sound/bank_sets.inc.c
        ${SM64COOPDX_TEXTURE_TARGETS}
    )

    set(_generated_outputs
        ${_generated_c_sources}
        "${SM64COOPDX_BUILD_ROOT}/include/level_headers.h"
        "${SM64COOPDX_BUILD_ROOT}/text/us/define_courses.inc.c"
        "${SM64COOPDX_BUILD_ROOT}/text/us/define_text.inc.c"
        "${SM64COOPDX_BUILD_ROOT}/sound/sound_data.ctl.inc.c"
        "${SM64COOPDX_BUILD_ROOT}/sound/sound_data.tbl.inc.c"
        "${SM64COOPDX_BUILD_ROOT}/sound/sequences.bin.inc.c"
        "${SM64COOPDX_BUILD_ROOT}/sound/bank_sets.inc.c"
        ${SM64COOPDX_TEXTURE_OUTPUTS}
    )

    set(_prebuild_inputs
        "${SM64COOPDX_ROOT}/Makefile"
        "${SM64COOPDX_ROOT}/Makefile.split"
        "${SM64COOPDX_ROOT}/util.mk"
        "${SM64COOPDX_ROOT}/levels/level_rules.mk"
        "${SM64COOPDX_ROOT}/levels/level_defines.h"
        "${SM64COOPDX_ROOT}/levels/level_headers.h.in"
        "${SM64COOPDX_ROOT}/assets/demo_data.json"
        "${SM64COOPDX_ROOT}/text/define_courses.inc.c"
        "${SM64COOPDX_ROOT}/text/define_text.inc.c"
        "${SM64COOPDX_ROOT}/text/us/courses.h"
        "${SM64COOPDX_ROOT}/text/us/dialogs.h"
        "${SM64COOPDX_ROOT}/sound/sound_data_compressed.ctl"
        "${SM64COOPDX_ROOT}/sound/sound_data_compressed.tbl"
        "${SM64COOPDX_ROOT}/sound/sequences_compressed.bin"
        "${SM64COOPDX_ROOT}/sound/bank_sets_compressed"
        ${SM64COOPDX_TEXTURE_INPUTS}
    )

    file(GLOB _anim_inputs CONFIGURE_DEPENDS
        "${SM64COOPDX_ROOT}/assets/anims/*.inc.c"
    )
    file(GLOB _demo_inputs CONFIGURE_DEPENDS
        "${SM64COOPDX_ROOT}/assets/demos/*.bin"
    )
    list(APPEND _prebuild_inputs ${_anim_inputs} ${_demo_inputs})

    file(TO_CMAKE_PATH "${SM64COOPDX_ROOT}" _make_root)
    string(JOIN " " _make_targets_arg ${_make_targets})
    set(_make_command
        "cd '${_make_root}' && make VERSION=us TARGET_N64=0 COOPNET=0 DISCORD_SDK=0 UPDATER=0 BUILD_PROGRAMS='n64graphics n64graphics_ci textconv' ${_make_targets_arg}"
    )

    set_source_files_properties(${_generated_c_sources} PROPERTIES GENERATED TRUE)

    add_custom_command(
        OUTPUT ${_generated_outputs}
        COMMAND "${SM64COOPDX_MSYS_BASH}" -lc "${_make_command}"
        DEPENDS ${_prebuild_inputs}
        WORKING_DIRECTORY "${SM64COOPDX_ROOT}"
        COMMENT "Generating sm64coopdx assets through the upstream Makefile"
        VERBATIM
    )

    add_custom_target(${target_name} DEPENDS ${_generated_outputs})

    set(${generated_sources_out} ${_generated_c_sources} PARENT_SCOPE)
endfunction()
