#ifndef _DEBUG_H_INCLUDED_
#define _DEBUG_H_INCLUDED_

#define LOG_DEBUG 0
#define LOG_INFO  1
#define LOG_WARN  2
#define LOG_ERROR 3
#define LOG_FATAL 4

#ifdef LOG_LEVEL
#ifndef LOG_PRINTF
#include <stdio.h>
#define LOG_PRINTF(...) fprintf(stderr, __VA_ARGS__)
#endif
#define LOG(level, ...)     \
    if (level >= LOG_LEVEL) \
    LOG_PRINTF(__VA_ARGS__)
#else // ifdef LOG_LEVEL
#define LOG(level, ...)
#endif

#endif // _DEBUG_H_INCLUDED_