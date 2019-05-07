/*
 * =============================================================================
 *
 *       Filename:  my_title.h
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
#ifndef _MY_TITLE_H
#define _MY_TITLE_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include "my_controls.h"
#include "commongdi.h"
#define CTRL_MYTITLE         ("mytitle")

	enum {
        MSG_MYTITLE_SET_TITLE = MSG_USER + 1,
    };

	enum {  // 左边类型
		MYTITLE_LEFT_EXIT, // 退出按键
	};
	enum {  // 右边类型
		MYTITLE_RIGHT_NULL,  // 空
		MYTITLE_RIGHT_TEXT,  // 文字
		MYTITLE_RIGHT_SWICH, // 图片-开关
		MYTITLE_RIGHT_ADD,   // 图片-添加
	};


	typedef struct {
        char text[64];      // 中间文字
        char text_right[64];// 右边文字
        PLOGFONT   font; // 字体
		int font_color;  // 文字颜色
		int bkg_color;  // 背景颜色
		int flag_left; 	// 左边类型
		int flag_right; // 右边类型
	}MyTitleCtrlInfo;

	typedef struct _MyCtrlTitle{
		HWND idc;		// 控件ID
		int flag_left; 	// 左边类型
		int flag_right; // 右边类型
		int16_t x,y,w,h;
        const char *text; // 中间文字
        const char *text_right; // 右边文字
		int font_color;   // 文字颜色
		int bkg_color;    // 背景颜色
        PLOGFONT   font;  // 字体
		NOTIFPROC notif_proc; 	// 回调函数
	}MyCtrlTitle;

	HWND createMyTitle(HWND hWnd,MyCtrlTitle *);
    MyControls *my_title;
	void initMyTitle(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
