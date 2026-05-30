#pragma once

// MSVC names these with underscores so giving upstream the POSIX spellings it expects
#ifdef _MSC_VER
#include <string.h>
#define strcasecmp _stricmp
#define strncasecmp _strnicmp
#endif
