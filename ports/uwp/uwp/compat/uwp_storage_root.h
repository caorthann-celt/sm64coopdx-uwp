#pragma once

// Pick the writable game root for saves, mods, and config
#include <stdbool.h>
#include <stddef.h>

#ifdef __cplusplus
extern "C" {
#endif

bool uwp_storage_get_user_path(char* dst, size_t dst_size);

#ifdef __cplusplus
}
#endif
