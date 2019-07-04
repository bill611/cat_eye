/*
 * =============================================================================
 *
 *       Filename:  hal_battery.c
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
#include <stdlib.h>

#include "hal_battery.h"
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
int halBatteryGetEle(void)
{
#ifdef X86
	return 10;
#else
	char data[4] = {0};
	int fd = open("/sys/class/power_supply/battery/capacity",O_RDONLY);
	if (fd != -1) {
		read(fd,data,sizeof(data));	
		close(fd);
	}
	int ret = atoi(data);
	// printf("ele:%s:%d\n", data,ret);
	return ret;
#endif
}

int halBatteryGetState(void)
{
#ifdef X86
		return BATTERY_NORMAL;	
#else
	char data[16] = {0};
	int fd = open("/sys/class/power_supply/battery/status",O_RDONLY);
	if (fd != -1) {
		read(fd,data,sizeof(data));	
		close(fd);
	}
	// printf("state:%s\n", data);
	if (strncmp(data,"Charging",strlen("Charging")) == 0) {
		return BATTERY_CHARGING;	
	} else {
		return BATTERY_NORMAL;	
	}
#endif
}
