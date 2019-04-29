/*
 * =============================================================================
 *
 *       Filename:  my_static.h
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
#ifndef _MY_STATIC_H
#define _MY_STATIC_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include "my_controls.h"
#include "commongdi.h"
#define CTRL_MYSTATIC         ("mystatic")

	enum {
        MSG_MYSTATIC_SET_TITLE = MSG_USER + 1,
    };

	enum {  // 控件类型
		MYSTATIC_TYPE_TEXT,
		MYSTATIC_TYPE_TEXT_AND_IMG,
	};


	typedef struct {
        char text[64];      // 文字
        PLOGFONT   font;       // 字体
		int font_color;  // 文字颜色
		int bkg_color;  // 背景颜色
		int flag; 		// 类型
		BITMAP *image;	// 图片
	}MyStaticCtrlInfo;

	typedef struct _MyCtrlStatic{
		HWND idc;		// 控件ID
		int flag; 		// 类型
		int16_t x,y,w,h;
        const char *text;             // 文字
		int font_color;  // 文字颜色
		int bkg_color;  // 背景颜色
		char *img_name; 	// 常态图片名字,不带扩展名,完整路径由循环赋值
        PLOGFONT   font;       // 字体
		BITMAP image;	// 正常状态图片
	}MyCtrlStatic;

	HWND createMyStatic(HWND hWnd,MyCtrlStatic *);
    MyControls *my_static;
	void initMyStatic(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
