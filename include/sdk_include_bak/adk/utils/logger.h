/*
 * Copyright (C) 2019 Rockchip Electronics Co., Ltd.
 *
 * This software is available to you under a choice of one of two
 * licenses.  You may choose to be licensed under the terms of the GNU
 * General Public License (GPL), available from the file
 * COPYING in the main directory of this source tree, or the
 * OpenIB.org BSD license below:
 *
 *     Redistribution and use in source and binary forms, with or
 *     without modification, are permitted provided that the following
 *     conditions are met:
 *
 *      - Redistributions of source code must retain the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer.
 *
 *      - Redistributions in binary form must reproduce the above
 *        copyright notice, this list of conditions and the following
 *        disclaimer in the documentation and/or other materials
 *        provided with the distribution.
 *
 * THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND,
 * EXPRESS OR IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF
 * MERCHANTABILITY, FITNESS FOR A PARTICULAR PURPOSE AND
 * NONINFRINGEMENT. IN NO EVENT SHALL THE AUTHORS OR COPYRIGHT HOLDERS
 * BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER LIABILITY, WHETHER IN AN
 * ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM, OUT OF OR IN
 * CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
 * SOFTWARE.
 */

#ifndef ADK_UTILS_LOGGER_H_
#define ADK_UTILS_LOGGER_H_

#ifdef DEBUG
#define ADK_DEBUG_DEFAULT_LEVEL 2
#else
#define ADK_DEBUG_DEFAULT_LEVEL 0
#endif

namespace rk {

class Logger {
public:
    Logger();
    ~Logger();

    typedef enum {
        kErrorLevel    = 0,
        kWarningLevel  = 1,
        kDebugLevel    = 2,
        kInfoLevel     = 3,
    } DebugLevel;

    static void Error(const char* tag, const char* format, ...);
    static void Warning(const char* tag, const char* format, ...);
    static void Debug(const char* tag, const char* format, ...);
    static void Info(const char* tag, const char* format, ...);

    static DebugLevel debug_level(void) {
        return _debug_level;
    }

    static void set_debug_level(const DebugLevel level) {
        _debug_level = level;
    }

private:
    static DebugLevel _debug_level;
};

#ifndef TAG
#define TAG "sys"
#endif

#define pr_err(format, ...) rk::Logger::Error(TAG, format, ##__VA_ARGS__)
#define pr_dbg(format, ...) rk::Logger::Debug(TAG, format, ##__VA_ARGS__)
#define pr_warn(format, ...) rk::Logger::Warning(TAG, format, ##__VA_ARGS__)
#define pr_info(format, ...) rk::Logger::Info(TAG, format, ##__VA_ARGS__)

#define debug_func_enter() rk::Logger::Info(__func__, "enter\n")
#define debug_func_exit()  rk::Logger::Info(__func__, "exit\n")

} // namespace rk

#endif // ADK_UTILS_LOGGER_H_
