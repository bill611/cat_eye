/*
 * =============================================================================
 *
 *       Filename:  hal_watchdog.c
 *
 *    Description:  硬件层 看门狗接口
 *
 *        Version:  1.0
 *        Created:  2018-12-12 15:51:59
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
/* ---------------------------------------------------------------------------*
 *                      include head files
 *----------------------------------------------------------------------------*/
#include <stdio.h>
#include <fcntl.h>
#include <unistd.h>
#include <string.h>
#include <sys/ioctl.h>
#include <linux/watchdog.h>

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static int fd = 0;
void halWatchDogOpen(void)
{
#ifndef WATCHDOG_DEBUG
	if(fd > 0) {
		return;
	}

	fd = open("/dev/watchdog", O_WRONLY);
	if (fd == -1) {
		perror("watchdog");
	} else {
		printf("Init WatchDog!!!!!!!!!!!!!!!!!!\n");
	}
#endif
}

void halWatchDogFeed(void)
{
#ifndef WATCHDOG_DEBUG
	if(fd <= 0) {
		return;
	}
	ioctl(fd, WDIOC_KEEPALIVE);
#endif
}

void halWatchDogClose(void)
{
#ifndef WATCHDOG_DEBUG
	if(fd <= 0) {
		return;
	}
	char * closestr="V";
	write(fd,closestr,strlen(closestr));
	close(fd);
	fd = -2;
#endif
}
