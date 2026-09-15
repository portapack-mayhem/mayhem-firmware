#ifndef _INCLUDE_DEBUG_H_
#define _INCLUDE_DEBUG_H_

// Debug stub for Portapack baseband
// All LOG macros are disabled (no-op)

#define LOG_FATAL 0
#define LOG_ERROR 1
#define LOG_WARN  2
#define LOG_INFO  3
#define LOG_DEBUG 4
#define LOG_TRACE 5

// Disable all logging in baseband (no printf available)
#define LOG(level, ...) do { } while(0)

#endif // _INCLUDE_DEBUG_H_
