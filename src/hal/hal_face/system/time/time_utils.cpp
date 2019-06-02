/*
 * Rockchip App
 *
 * Copyright (C) 2017 Rockchip Electronics Co., Ltd.
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

#include "time_utils.h"

#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <linux/rtc.h>
#include <sys/ioctl.h>
#include <sys/stat.h>
#include <sys/time.h>
#include <sys/types.h>

static int rtc_set_time(const struct tm* tm_time)
{
    int rtc_handle = -1;
    int ret = 0;
    struct rtc_time rtc_tm;

    if (tm_time == NULL) {
        return -1;
    }
    rtc_handle = open("/dev/rtc0", O_RDWR, 0);
    if (rtc_handle < 0) {
        //      db_error("open /dev/rtc0 fail");
        return -1;
    }
    memset(&rtc_tm, 0, sizeof(rtc_tm));
    rtc_tm.tm_sec = tm_time->tm_sec;
    rtc_tm.tm_min = tm_time->tm_min;
    rtc_tm.tm_hour = tm_time->tm_hour;
    rtc_tm.tm_mday = tm_time->tm_mday;
    rtc_tm.tm_mon = tm_time->tm_mon;
    rtc_tm.tm_year = tm_time->tm_year;
    rtc_tm.tm_wday = tm_time->tm_wday;
    rtc_tm.tm_yday = tm_time->tm_yday;
    rtc_tm.tm_isdst = tm_time->tm_isdst;
    ret = ioctl(rtc_handle, RTC_SET_TIME, &rtc_tm);
    if (ret < 0) {
        //       db_error("rtcSetTime fail");
        close(rtc_handle);
        return -1;
    }

    close(rtc_handle);
    return 0;
}

int system_set_datetime(struct tm* ptm)
{
    time_t timep;
    struct timeval tv;

    //    open("/dev/rtc0",O_RDWR);
    timep = mktime(ptm);
    tv.tv_sec = timep;
    tv.tv_usec = 0;

    if (settimeofday(&tv, NULL) < 0)
        return -1;

    time_t t = time(NULL);
    struct tm* local = localtime(&t);
    rtc_set_time(local);
    return 0;
}
