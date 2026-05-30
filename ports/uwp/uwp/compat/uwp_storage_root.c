#include "uwp_storage_root.h"

#include <stdio.h>
#include <string.h>
#include <windows.h>
#include <SDL.h>

#include "fs/fs.h"
#include "pc/platform.h"

static bool uwp_storage_copy_path(char* dst, size_t dst_size, const char* src) {
    if (!src || snprintf(dst, dst_size, "%s", src) < 0) { return false; }

    const size_t len = strlen(dst);
    if (len > 0 && (dst[len - 1] == *PATH_SEPARATOR || dst[len - 1] == *PATH_SEPARATOR_ALT)) {
        dst[len - 1] = '\0';
    }

    return dst[0] != '\0';
}

static bool uwp_storage_has_rom(const char* root) {
    char pattern[SYS_MAX_PATH] = { 0 };
    if (snprintf(pattern, sizeof(pattern), "%s\\*.z64", root) < 0) { return false; }

    WIN32_FIND_DATAA data;
    HANDLE handle = FindFirstFileA(pattern, &data);
    if (handle == INVALID_HANDLE_VALUE) { return false; }

    FindClose(handle);
    return true;
}

static bool uwp_storage_is_writable(const char* root) {
    char path[SYS_MAX_PATH] = { 0 };
    if (snprintf(path, sizeof(path), "%s\\.uwp-write-test", root) < 0) { return false; }

    FILE* file = fopen(path, "wb");
    if (!file) { return false; }

    fputs("ok", file);
    fclose(file);
    remove(path);
    return true;
}

static void uwp_storage_ensure_dir(const char* path) {
    if (!fs_sys_dir_exists(path)) { fs_sys_mkdir(path); }
}

static void uwp_storage_prepare_user_folders(const char* root) {
    static const char* folders[] = {
        "mods",
        "dynos",
        "dynos\\packs",
        "sav",
        NULL,
    };

    for (int i = 0; folders[i] != NULL; i++) {
        char path[SYS_MAX_PATH] = { 0 };
        if (snprintf(path, sizeof(path), "%s\\%s", root, folders[i]) < 0) { continue; }
        uwp_storage_ensure_dir(path);
    }
}

static bool uwp_storage_try_external_path(char* dst, size_t dst_size) {
    // If ROM lives on E:\sm64coopdx, treat that as the user root
    const char* external_root = "E:\\sm64coopdx";
    if (!fs_sys_dir_exists(external_root)) { return false; }
    if (!uwp_storage_has_rom(external_root)) { return false; }
    if (!uwp_storage_is_writable(external_root)) { return false; }

    return uwp_storage_copy_path(dst, dst_size, external_root);
}

static bool uwp_storage_try_local_path(char* dst, size_t dst_size) {
    const char* sdl_path = SDL_WinRTGetFSPathUTF8(SDL_WINRT_PATH_LOCAL_FOLDER);
    if (sdl_path == NULL) { return false; }

    return uwp_storage_copy_path(dst, dst_size, sdl_path);
}

bool uwp_storage_get_user_path(char* dst, size_t dst_size) {
    char local_root[SYS_MAX_PATH] = { 0 };
    if (!uwp_storage_try_local_path(local_root, sizeof(local_root))) {
        return false;
    }

    const bool using_external = uwp_storage_try_external_path(dst, dst_size);
    if (!using_external && !uwp_storage_copy_path(dst, dst_size, local_root)) {
        return false;
    }

    uwp_storage_prepare_user_folders(local_root);
    if (using_external) {
        uwp_storage_prepare_user_folders(dst);
    }
    return true;
}
