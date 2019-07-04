/*
 * =============================================================================
 *
 *       Filename:  my_battery.h
 *
 *    Description:  自定义静态控件
 *
 *        Version:  1.0
 *        Created:  2019-04-23 19:46:14
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _MY_BATTERY_H
#define _MY_BATTERY_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include "my_controls.h"
#include "commongdi.h"
#define CTRL_MYBATTERY         ("mybattery")

	enum {
        MSG_SET_QUANTITY = MSG_USER + 1,
        MSG_SET_STATUS,
    };


	typedef struct {
        int ele_quantity; // 电量
        PLOGFONT   font; // 字体
        int	state; // 状态,0正常，1充电
	}MyBatteryCtrlInfo;

	typedef struct _MyCtrlBattery{
		HWND idc;		// 控件ID
		int16_t x,y,w,h;
        PLOGFONT   font;  // 字体
        int ele_quantity; // 电量
        int	state; // 状态,0正常，1充电
	}MyCtrlBattery;

	HWND createMyBattery(HWND hWnd,MyCtrlBattery *);
    extern MyControls *my_battery;
	void initMyBattery(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
