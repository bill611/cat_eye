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
        MSG_MYTITLE_SET_SWICH,
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
    enum {
        MYTITLE_SWICH_OFF = 0,
        MYTITLE_SWICH_ON,
    };

	enum {  // 按键类型
		MYTITLE_BUTTON_NULL = 0, 
		MYTITLE_BUTTON_EXIT, 
		MYTITLE_BUTTON_ADD, 
		MYTITLE_BUTTON_SWICH, 
	};

    typedef struct {
        RECT rc;    // 按钮位置
        int state;  // 按钮状态
        BITMAP *image_nor; //按下图片 无则为null
        BITMAP *image_pre; //抬起图片
    }MyTitleButton;

	typedef struct {
        char text[64];      // 中间文字
        char text_right[64];// 右边文字
        PLOGFONT   font; // 字体
		int font_color;  // 文字颜色
		int bkg_color;  // 背景颜色
		int flag_left; 	// 左边类型
		int flag_right; // 右边类型
		MyTitleButton bt_exit;   //退出按钮
		MyTitleButton bt_add;    //添加按钮
		MyTitleButton bt_swich;  //开关按钮
        int click_x,click_y;     //点击坐标
	}MyTitleCtrlInfo;

	typedef struct _MyCtrlTitle{
		HWND idc;		// 控件ID
		int flag_left; 	// 左边类型
		int flag_right; // 右边类型
		int16_t x,y,w,h;
        const char *text; // 中间文字
        const char *text_right; // 右边文字
		int font_color, bkg_color;   // 文字颜色  // 背景颜色
		NOTIFPROC notif_proc; 	// 回调函数
        PLOGFONT   font;  // 字体
	}MyCtrlTitle;

	HWND createMyTitle(HWND hWnd,MyCtrlTitle *);
    MyControls *my_title;
	void initMyTitle(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
