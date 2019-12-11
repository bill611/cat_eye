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
		MYBUTTON_TYPE_ONE_STATE = (1 << 0),
		MYBUTTON_TYPE_TWO_STATE = (1 << 1),
		MYBUTTON_TYPE_CHECKBOX  = (1 << 2),
		
		MYBUTTON_TYPE_TEXT_CENTER  = (1 << 10),  // 文字居中，否则在最底部
		MYBUTTON_TYPE_PRESS_COLOR  = (1 << 11),  // 背景是否为纯色，否则为图片
		MYBUTTON_TYPE_PRESS_TRANSLATE  = (1 << 12),  // 无背景色
		MYBUTTON_TYPE_TEXT_NULL  = (1 << 13),  // 无文字
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
		unsigned int color_nor,color_press;// 文字颜色，正常颜色与按下颜色,默认为白色
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
		int16_t w,h;
		unsigned  color_nor,color_press;// 文字颜色，正常颜色与按下颜色,默认为白色
	}MyCtrlButton;

	HWND createMyButton(HWND hWnd,MyCtrlButton *button);
    MyControls *my_button;
	void initMyButton(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
