#pragma once

#include <string>
#include <cinttypes>
#include <cstdio>
#include <cstdarg>
#include <time.h>
#include <cstring>
#if defined(UWP_BUILD)
#include <windows.h>
#endif

static void _debuglog_print_timestamp(void) {
    time_t ltime = time(NULL);
#if defined(_WIN32)
    char* str = asctime(localtime(&ltime));
#else
    struct tm ltime2 = { 0 };
    localtime_r(&ltime, &ltime2);
    char* str = asctime(&ltime2);
#endif
    printf("%.*s", (int)strlen(str) - 1, str);
}

static void _debuglog_print_log_type(std::string logType) {
    printf(" [%s] ", logType.c_str());
}

static void _debuglog_print_short_filename(std::string filename) {
    char* last = strrchr((char*)filename.c_str(), '/');
    if (last != NULL) {
        printf("%s: ", last + 1);
    }
    else {
        printf("???: ");
    }
}

static void _debuglog_print_log(std::string logType, std::string filename) {
    _debuglog_print_timestamp();
    _debuglog_print_log_type(logType);
    _debuglog_print_short_filename(filename);
}

#if defined(DISABLE_MODULE_LOG) or !defined(LOGGING)
#define LOG_DEBUG(...)
#define LOG_INFO(...)
#define LOG_ERROR(...)
#elif defined(UWP_BUILD)
static void _debuglog_output_debug_string(const char* logType, const char* filename, const char* format, ...) {
    char message[1024] = { 0 };
    char line[1200] = { 0 };

    va_list args;
    va_start(args, format);
    vsnprintf(message, sizeof(message), format, args);
    va_end(args);

    const char* shortFilename = strrchr(filename, '/');
    if (shortFilename == NULL) { shortFilename = strrchr(filename, '\\'); }
    shortFilename = (shortFilename != NULL) ? shortFilename + 1 : filename;

    snprintf(line, sizeof(line), "CoopNet %s %s: %s\n", logType, shortFilename, message);
    OutputDebugStringA(line);
}
#define LOG_DEBUG(...) ( _debuglog_output_debug_string("DEBUG", __FILE__, __VA_ARGS__) )
#define LOG_INFO(...)  ( _debuglog_output_debug_string("INFO",  __FILE__, __VA_ARGS__) )
#define LOG_ERROR(...) ( _debuglog_output_debug_string("ERROR", __FILE__, __VA_ARGS__) )
#else
#define LOG_DEBUG(...) ( _debuglog_print_log("DEBUG", __FILE__), printf(__VA_ARGS__), printf("\n") )
#define LOG_INFO(...)  ( _debuglog_print_log("INFO",  __FILE__), printf(__VA_ARGS__), printf("\n") )
#define LOG_ERROR(...) ( _debuglog_print_log("ERROR", __FILE__), printf(__VA_ARGS__), printf("\n") )
#endif
