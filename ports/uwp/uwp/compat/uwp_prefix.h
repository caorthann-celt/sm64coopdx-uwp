#pragma once

// Forced into the UWP build so upstream code sees the few POSIX bits it expects
#if !defined(__GNUC__) && !defined(__clang__)
#define __attribute__(x)
#endif

#ifndef _SSIZE_T_DEFINED
#include <BaseTsd.h>
typedef SSIZE_T ssize_t;
#define _SSIZE_T_DEFINED
#endif

#ifdef _MSC_VER
#ifndef _WINSOCK_DEPRECATED_NO_WARNINGS
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#endif

#include <ctype.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>

#ifdef UWP_BUILD
#include <SDL.h>
#define ShellExecuteA(...) ((void *)0)

#endif

#ifndef CLOCK_REALTIME
#define CLOCK_REALTIME 0
#endif

#ifndef CLOCK_MONOTONIC
#define CLOCK_MONOTONIC 1
#endif

static inline int clock_gettime(int clock_id, struct timespec *ts) {
    (void)clock_id;
    return timespec_get(ts, TIME_UTC) == TIME_UTC ? 0 : -1;
}

#ifndef strcasecmp
#define strcasecmp _stricmp
#endif

#ifndef strncasecmp
#define strncasecmp _strnicmp
#endif

#ifndef strtok_r
#define strtok_r strtok_s
#endif

#ifndef S_ISDIR
#define S_ISDIR(mode) (((mode) & _S_IFDIR) != 0)
#endif
#endif
