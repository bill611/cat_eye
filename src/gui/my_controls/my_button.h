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

#include "my_controls.h"
#include "commongdi.h"
#define CTRL_MYBUTTON         ("mybutton")

	enum {  // 按钮类型
		MYBUTTON_TYPE_ONE_STATE,
		MYBUTTON_TYPE_TWO_STATE,
		MYBUTTON_TYPE_CHECKBOX,
	};

	enum {
		BUT_NORMAL, // 正常
		BUT_CLICK,  // 按下
		BUT_DISABLED, // 不启用
	};

	enum {
		MYBUTTON_STATE_UNCHECK, // 非选择状态
		MYBUTTON_STATE_CHECK, // 选择状态
	};

	typedef struct {
        const char *text;      // 文字
        PLOGFONT   font;       // 字体
		BITMAP *image_normal;	// 正常状态图片
		BITMAP *image_press;	// 按下状态图片
		BITMAP *image_select;	// 选中图片
		BITMAP *image_unselect;	// 非选中图片
		int flag;  // 按扭类型
		int state; //BUTTON状态
		int check; // 选中状态
	}MyButtonCtrlInfo;

	typedef struct _MyCtrlButton{
		HWND idc;		// 控件ID
		int flag;		// 按钮类型
		char *img_name; 	// 常态图片名字,不带扩展名,完整路径由循环赋值
		int16_t x,y;
		NOTIFPROC notif_proc; 	// 回调函数
        PLOGFONT   font;       // 字体
		BITMAP image_normal;	// 正常状态图片
		BITMAP image_press;	// 按下状态图片
	}MyCtrlButton;

	HWND createMyButton(HWND hWnd,MyCtrlButton *button);
    MyControls *my_button;
	void initMyButton(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
