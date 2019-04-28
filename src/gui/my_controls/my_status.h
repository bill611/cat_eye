/*
 * =============================================================================
 *
 *       Filename:  my_status.h
 *
 *    Description:  自定义状态显示
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
#ifndef _MY_STATUS_H
#define _MY_STATUS_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include "my_controls.h"
#include "commongdi.h"
#define CTRL_MYSTATUS         ("mystatus")

#define MSG_MYSTATUS_SET_LEVEL (MSG_USER + 1)
	typedef struct {
		BITMAP *images;		// 状态图片数组
		int total_level;	// 总共几组状态
		int level;			// 当前状态
	}MyStatusCtrlInfo;

	typedef struct _MyCtrlStatus{
		HWND idc;		// 控件ID
		char *relative_img_path; // 图片相对位置
		char *img_name; 	// 常态图片名字,不带扩展名,完整路径由循环赋值
		int16_t x,y;
		int total_level;	// 总共几组状态
		BITMAP *images;		// 状态图片数组
	}MyCtrlStatus;

    HWND createMyStatus(HWND hWnd,MyCtrlStatus *ctrl);
    MyControls * my_status;
	void initMyStatus(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
