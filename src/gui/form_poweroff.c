/*
 * =============================================================================
 *
 *       Filename:  FormPowerOff.c
 *
 *    Description:  关机界面
 *
 *        Version:  1.0
 *        Created:  2016-02-23 15:32:24
 *       Revision:  1.0
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
/* ---------------------------------------------------------------------------*
 *                      include head files
 *----------------------------------------------------------------------------*/
#include <time.h>
#include <string.h>
#include "externfunc.h"
#include "debug.h"
#include "screen.h"
#include "config.h"

#include "form_base.h"
/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static int formPoweroffProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam);


/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/

enum {
	IDC_TIMER_1S  = IDC_FORM_POWEROFF_STATR,
    IDC_CONTTENT,

};

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/

static MY_CTRLDATA ChildCtrls [] = {
    STATIC_LB(0,280,1024,40,IDC_CONTTENT,"正在关机...",&font22,0xffffff),
};


static MY_DLGTEMPLATE DlgInitParam =
{
    WS_NONE,
	WS_EX_NONE ,//| WS_EX_AUTOSECONDARYDC,
    0,0,SCR_WIDTH,SCR_HEIGHT,
    "",
    0, 0,       //menu and icon is null
    sizeof(ChildCtrls)/sizeof(MY_CTRLDATA),
    ChildCtrls, //pointer to control array
    0           //additional data,must be zero
};

static FormBasePriv form_base_priv= {
	.name = "FPoweroff",
	.idc_timer = IDC_TIMER_1S,
	.dlgProc = formPoweroffProc,
	.dlgInitParam = &DlgInitParam,
};

static FormBase* form_base = NULL;

/* ---------------------------------------------------------------------------*/
/**
 * @brief formPoweroffProc 窗口回调函数
 *
 * @param hDlg
 * @param message
 * @param wParam
 * @param lParam
 *
 * @return
 */
/* ---------------------------------------------------------------------------*/
static int formPoweroffProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case MSG_TIMER:
			{
				if (wParam == IDC_TIMER_1S) {
					return 0;
				}
			} break;

		default:
			break;
	}
	if (form_base->baseProc(form_base,hDlg, message, wParam, lParam) == FORM_STOP)
		return 0;

    return DefaultDialogProc(hDlg, message, wParam, lParam);
}

int createFormPowerOff(HWND hMainWnd)
{
	HWND Form = Screen.Find(form_base_priv.name);
	if(Form) {
		ShowWindow(Form,SW_SHOWNORMAL);
		Screen.setCurrent(form_base_priv.name);
	} else {
		ShowWindow(Form,SW_SHOWNORMAL);
		form_base = formBaseCreate(&form_base_priv);
		return CreateMyWindowIndirectParam(form_base->priv->dlgInitParam,
				hMainWnd, form_base->priv->dlgProc, 0);
	}

	return 0;
}
int createFormPowerOffLowPower(void)
{
	createFormPowerOff(0);
	SendMessage(GetDlgItem(form_base->hDlg,IDC_CONTTENT),MSG_SETTEXT,0,(LPARAM)"电量过低，即将关机，请及时充电...");
}
int createFormPowerOffCammerError(void)
{
	createFormPowerOff(0);
	SendMessage(GetDlgItem(form_base->hDlg,IDC_CONTTENT),MSG_SETTEXT,0,(LPARAM)"镜头接线异常，即将关机，请接好后再重新开机...");
}
int createFormPowerOffCammerErrorSleep(void)
{
	createFormPowerOff(0);
	SendMessage(GetDlgItem(form_base->hDlg,IDC_CONTTENT),MSG_SETTEXT,0,(LPARAM)"镜头接线异常，即将进入休眠，请断开电源并接好镜头后再重新开机...");
}
