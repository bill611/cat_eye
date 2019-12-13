/*
 * =============================================================================
 *
 *       Filename:  my_scroll.h
 *
 *    Description:  自定义滚轮选择
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
#ifndef _MY_SCROLL_H
#define _MY_SCROLL_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include "my_controls.h"
#include "commongdi.h"
#define CTRL_MYSCROLL         ("myscroll")


	typedef struct {
        const char *text;      // 文字
		int flag;		// 类型
        PLOGFONT   font;       // 字体
		int index_start,index_end; // 数字范围
	}MyScrollCtrlInfo;

	typedef struct _MyCtrlScroll{
		HWND idc;		// 控件ID
		int flag;		// 类型
		char *text; 	// 数字旁的中文后缀
		int index_start,index_end; // 数字范围
        PLOGFONT   font;       // 字体
	}MyCtrlScroll;

	HWND createMyScroll(HWND hWnd,MyCtrlScroll *button);
	extern MyControls *my_scroll;
	void initMyScroll(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
