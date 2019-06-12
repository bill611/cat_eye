//
// Copyright 2005 The Android Open Source Project
//
// C/C++ logging functions.  See the logging documentation for API details.
//
// We'd like these to be available from C code (in case we import some from
// somewhere), so this has a C interface.
//
// The output will be correct when the log file is shared between multiple
// threads and/or multiple processes so long as the operating system
// supports O_APPEND.  These calls have mutex-protected data structures
// and so are NOT reentrant.  Do not use LOG in a signal handler.
//
#ifndef BASE_LOG_H_
#define BASE_LOG_H_

#include <stdio.h>

// ---------------------------------------------------------------------

/*
 * Normally we strip LOGV (VERBOSE messages) from release builds.
 * You can modify this (for example with "#define LOG_NDEBUG 0"
 * at the top of your source file) to change that behavior.
 */
#ifndef LOG_NDEBUG
#ifdef NDEBUG
#define LOG_NDEBUG 1
#else
#define LOG_NDEBUG 0
#endif
#endif

/*
 * This is the local tag used for the following simplified
 * logging macros.  You can change this preprocessor definition
 * before using the other macros to change the tag.
 */

#ifndef LOGD
#if LOG_NDEBUG
#define LOGD(...)   ((void)0)
#else
#define LOGD(...) printf(__VA_ARGS__)
#endif
#endif

#ifndef LOGE
#if LOG_NDEBUG
#define LOGE(...)   ((void)0)
#else
#define LOGE(...) printf(__VA_ARGS__)
#endif
#endif

#endif // BASE_LOG_H_
