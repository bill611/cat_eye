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
#define SAVE_ELE 5

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
int halBatteryGetEle(void)
{
#ifdef X86
	return 10;
#else
	char data[4] = {0};
	int disp_value = 0;
	int fd = open("/sys/class/power_supply/battery/capacity",O_RDONLY);
	if (fd != -1) {
		read(fd,data,sizeof(data));	
		close(fd);
	}
	int real_value = atoi(data);
	// 为保持有最低安全电量，当实际电量低于SAVE_ELE时，显示设置为0
	if (real_value <= SAVE_ELE) {
		disp_value = SAVE_ELE;
	} else {
		disp_value = 100 * (real_value  - SAVE_ELE)/ (100 - SAVE_ELE) ;
	}
	// printf("re:%d,dis:%d\n", real_value,disp_value);
	return disp_value;
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
