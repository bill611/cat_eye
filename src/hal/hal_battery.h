/*
 * =============================================================================
 *
 *       Filename:  hal_battery.h
 *
 *    Description:  硬件层 看门狗接口
 *
 *        Version:  virsion
 *        Created:  2018-12-12 15:52:30 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _HAL_BATTERY_H
#define _HAL_BATTERY_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

	enum {
		BATTERY_NORMAL,
		BATTERY_CHARGING,
	};

	int halBatteryGetEle(void);
	int halBatteryGetState(void);
    int halBatteryGetCurrent(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
