/*
 * =====================================================================================
 *
 *       Filename:  FormBase.h
 *
 *    Description:  设置类基本窗口框架
 *
 *        Version:  1.0
 *        Created:  2016-02-19 15:24:17 
 *       Revision:  1.0
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =====================================================================================
 */
#ifndef _FORM_BASE_H
#define _FORM_BASE_H

#include "commongdi.h"
#include "my_dialog.h"

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#define FORM_TIMER_1S 100  // 定时器1秒

#define FORM_SETTING_ONTIME      5 // 设置界面10秒无操作，则关闭窗口

	enum {
		FORM_STOP = 0,
		FORM_CONTINUE = 1,
	};

	typedef struct _FormBasePriv {
		char *name;
		BITMAP *bmp_bkg;
		MY_DLGTEMPLATE *dlgInitParam;
		int idc_timer;
		int auto_close_time;  // 对话框自动关闭时间
		int auto_close_time_set;  // 对话框自动关闭时间设置，默认为FORM_SETTING_ONTIME
		int show_video;		  // 界面是否显示视频
		int (*dlgProc)(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);

		void (*initPara)(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
		void (*callBack)(void);
	}FormBasePriv;

	typedef struct _FormBase {
		FormBasePriv *priv;
		int hDlg; // 当前对话框的句柄
		int auto_close_time_set;  // 对话框自动关闭时间设置，默认为FORM_SETTING_ONTIME

		int (*baseProc)(struct _FormBase *this,HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
	}FormBase;

	FormBase * formBaseCreate(FormBasePriv *priv);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
