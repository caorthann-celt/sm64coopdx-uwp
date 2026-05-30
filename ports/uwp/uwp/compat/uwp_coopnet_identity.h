#pragma once

// Keeps old weak UWP dest ids from following players around forever
#include <stdbool.h>
#include <stdint.h>

#ifdef __cplusplus
extern "C" {
#endif

bool uwp_coopnet_refresh_dest_id_if_needed(uint64_t* dest_id);

#ifdef __cplusplus
}
#endif
