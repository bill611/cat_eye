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

#define FORM_SETTING_ONTIME      5 // 设置界面60秒无操作，则关闭窗口

	enum {
		FORM_STOP = 0,
		FORM_CONTINUE = 1,
	};

	typedef struct _FormBasePriv {
		HWND hwnd;
		char *name;
		BITMAP *bmp_bkg;
		MY_DLGTEMPLATE *dlgInitParam;
		int idc_timer;
		int (*dlgProc)(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);

		void (*initPara)(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
		void (*callBack)(void);
	}FormBasePriv;

	typedef struct _FormBase {
		FormBasePriv *priv;
		int auto_close_time;

		int (*baseProc)(struct _FormBase *this,HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
	}FormBase;

	FormBase * formBaseCreate(FormBasePriv *priv);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
