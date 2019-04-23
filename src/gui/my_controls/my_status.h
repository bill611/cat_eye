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

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include "commongdi.h"
#define CTRL_MYSTATUS         ("mystatus")

	typedef struct {
		BITMAP *images;		// 状态图片数组
		int total_level;	// 总共几组状态
		int level;			// 当前状态
	}MyStatusCtrlInfo;

	BOOL myStatusRegist(void);
	void myStatusCleanUp(void);

	// HWND createSkinButton(HWND hWnd,MgCtrlButton *button, int display, int mode);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
