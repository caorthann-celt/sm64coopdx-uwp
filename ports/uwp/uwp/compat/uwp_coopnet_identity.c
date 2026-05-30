#include "uwp_coopnet_identity.h"

#include <windows.h>
#include <bcrypt.h>

// Use the proper Windows random source instead of the old rand style value.
static bool uwp_coopnet_generate_dest_id(uint64_t* dest_id) {
    if (dest_id == NULL) { return false; }

    uint64_t value = 0;
    if (BCryptGenRandom(NULL, (PUCHAR)&value, sizeof(value), BCRYPT_USE_SYSTEM_PREFERRED_RNG) < 0) {
        return false;
    }

    if (value == 0) {
        value = 1;
    }

    *dest_id = value;
    return true;
}

static bool uwp_coopnet_dest_id_looks_weak(uint64_t dest_id) {
    if (dest_id == 0) { return true; }

    // Older UWP builds used MSVC rand(), leaving the high bits empty.
    return (dest_id >> 47) == 0;
}

bool uwp_coopnet_refresh_dest_id_if_needed(uint64_t* dest_id) {
    if (dest_id == NULL) { return false; }
    if (!uwp_coopnet_dest_id_looks_weak(*dest_id)) { return false; }
    return uwp_coopnet_generate_dest_id(dest_id);
}
