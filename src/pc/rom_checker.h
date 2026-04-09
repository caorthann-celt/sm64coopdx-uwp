#include "types.h"

#ifdef __cplusplus
extern "C" {
#endif

extern bool gRomIsValid;
extern char gRomFilename[];

void legacy_folder_handler(void);

bool main_rom_handler(void);
void rom_on_drop_file(const char *path);

#ifdef __cplusplus
}
#endif
