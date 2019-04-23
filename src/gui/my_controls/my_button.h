/*
 * =============================================================================
 *
 *       Filename:  my_button.h
 *
 *    Description:  自定义皮肤按键
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
#ifndef _MY_BUTTON_H
#define _MY_BUTTON_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include "commongdi.h"
#define CTRL_MYBUTTON         ("mybutton")

	enum {
		BUT_NORMAL, // 正常
		BUT_CLICK,  // 按下
		BUT_DISABLED, // 不启用
	};
	enum {
		BUT_STATE_UNSELECT, // 非选择状态
		BUT_STATE_SELECT, // 选择状态
	};

	struct ButtonSelect {
		int mode; // 0正常模式 1check模式
		int state;
	};

	typedef struct {
		BITMAP *image_normal;	// 正常状态图片
		BITMAP *image_press;	// 按下状态图片
		BITMAP *image_select;	// 选中图片
		BITMAP *image_unselect;	// 非选中图片
		struct ButtonSelect select; //设置模式时是否勾选
		int state;		//BUTTON状态
	}MyButtonCtrlInfo;

	typedef struct _MyCtrlButton{
		HWND idc;		// 控件ID
		char *img_name; 	// 常态图片名字,不带扩展名,完整路径由循环赋值
		int16_t x,y,w,h;
		NOTIFPROC notif_proc; 	// 回调函数
		BITMAP image_normal;	// 正常状态图片
		BITMAP image_press;	// 按下状态图片
	}MyCtrlButton;

	BOOL myButtonRegist(void);
	void myButtonCleanUp(void);

	void myButtonBmpsLoad(MyCtrlButton *controls,char *local_path);
	HWND createSkinButton(HWND hWnd,MyCtrlButton *button, int display, int mode);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
