#pragma once

// DynOS is C++ but level scripts are C symbols
#ifdef __cplusplus
extern "C" {
#include "types.h"
extern const LevelScript level_castle_grounds_entry[];
extern const LevelScript level_castle_inside_entry[];
extern const LevelScript level_castle_courtyard_entry[];
}
#endif
