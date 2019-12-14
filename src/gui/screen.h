/*
 * =============================================================================
 *
 *       Filename:  screen.h
 *
 *    Description:  窗口链表
 *
 *        Version:  1.0
 *        Created:  2019-04-20 15:09:14 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _SCREEN_H
#define _SCREEN_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include "commongdi.h"

	enum {
		MSG_ENABLE_WINDOW  = MSG_USER + 100,
		MSG_DISABLE_WINDOW,
		MSG_FORM_SETTING_TIME_SET_TIME,
		MSG_FORM_SETTING_TIME_GET_TIME,
	};

	typedef struct _Formclass {
		HWND hWnd;
		char Class[16];
		struct _Formclass * next;
	}FormClass;

	typedef struct _ScreenForm {
		HWND hMainWnd;												//主窗口 HWND hUpdate;
		int Width;
		int Height;
		int Count;													//合计数
		FormClass * head,*tail;										//窗口链表头与尾
		FormClass *current;											//窗口链表头与尾
		BOOL (*Add)(HWND hWnd,const char *Class);					//添加窗口
		BOOL (*Del)(HWND hWnd);										//删除窗口
		HWND (*Find)(const char *Class);							//查找窗口
		void (*setCurrent)(const char *Class);						//设置当前窗口
		HWND (*getCurrent)(void);									//拿到当前窗口
		void (*ReturnMain)(void);									//返回主窗口
		void (*foreachForm)(int iMsg, WPARAM wParam, LPARAM lParam); //遍历所有窗口发送消息
	} ScreenForm;

	void screenInit(void);
	extern ScreenForm Screen;

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
