/*
 * =============================================================================
 *
 *       Filename:  FormUpdate.c
 *
 *    Description:  升级界面
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
#include "my_update.h"
#include "my_video.h"

#include "form_base.h"
/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
extern void screenAutoCloseStop(void);

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static int formUpdateProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam);
static void initPara(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
int createFormUpdate(HWND hMainWnd);


/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/

enum {
	IDC_TIMER_1S  = IDC_FORM_UPDATE_STATR,
    IDC_CONTTENT,
    IDC_POS,

};

#define WORD_DOWNLOAD "正在下载更新程序,请勿关闭电源..."
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static int auto_close = 0;

static MY_CTRLDATA ChildCtrls [] = {
    STATIC_LB(0,280,1024,40,IDC_CONTTENT,WORD_DOWNLOAD,&font22,0xffffff),
    STATIC_LB(0,330,1024,40,IDC_POS,"",&font22,0xffffff),
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
	.name = "FUpdate",
	.idc_timer = IDC_TIMER_1S,
	.dlgProc = formUpdateProc,
	.dlgInitParam = &DlgInitParam,
	.initPara =  initPara,
};

static FormBase* form_base = NULL;

static void interfaceUiUpdateStart(void)
{
	auto_close = 0;
	my_video->hideVideo();
	screensaverSet(1);
	screenAutoCloseStop();
	createFormUpdate(0);
}

static void interfaceUiUpdateStop(void)
{
	// ShowWindow(form_base->hDlg,SW_HIDE);
}

static void interfaceUiUpdateFail(void)
{
	SendMessage(GetDlgItem(form_base->hDlg,IDC_CONTTENT),MSG_SETTEXT,0,(LPARAM)"更新失败..");
	auto_close = 1;
	// ShowWindow(form_base->hDlg,SW_HIDE);
}

static void interfaceUiUpdateSuccess(void)
{
	SendMessage(GetDlgItem(form_base->hDlg,IDC_CONTTENT),MSG_SETTEXT,0,(LPARAM)"更新成功,系统即将重启,请勿关闭电源...");
}

static void interfaceUiUpdateDowloadSuccess(void)
{
	SendMessage(GetDlgItem(form_base->hDlg,IDC_CONTTENT),MSG_SETTEXT,0,(LPARAM)"下载完成,正在检测升级包...");
}

static void interfaceUiUpdatePos(int pos)
{
	char buf[16] = {0};
	sprintf(buf,"%d%%",pos);
	SendMessage(GetDlgItem(form_base->hDlg,IDC_POS),MSG_SETTEXT,0,(LPARAM)buf);
}


/* ----------------------------------------------------------------*/
/**
 * @brief initPara 初始化参数
 *
 * @param hDlg
 * @param message
 * @param wParam
 * @param lParam
 */
/* ----------------------------------------------------------------*/
static void initPara(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
	if (my_update) {
		my_update->interface->uiUpdateStart = interfaceUiUpdateStart;
		my_update->interface->uiUpdateSuccess = interfaceUiUpdateSuccess;
		my_update->interface->uiUpdateDownloadSuccess = interfaceUiUpdateDowloadSuccess;
		my_update->interface->uiUpdateFail = interfaceUiUpdateFail;
		my_update->interface->uiUpdatePos = interfaceUiUpdatePos;
		my_update->interface->uiUpdateStop = interfaceUiUpdateStop;
	}	
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief formUpdateProc 窗口回调函数
 *
 * @param hDlg
 * @param message
 * @param wParam
 * @param lParam
 *
 * @return
 */
/* ---------------------------------------------------------------------------*/
static int formUpdateProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case MSG_TIMER:
			{
				if (wParam == IDC_TIMER_1S && auto_close == 0) {
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

int createFormUpdate(HWND hMainWnd)
{
	HWND Form = Screen.Find(form_base_priv.name);
	if(Form) {
		ShowWindow(Form,SW_SHOWNORMAL);
		SendMessage(GetDlgItem(form_base->hDlg,IDC_POS),MSG_SETTEXT,0,(LPARAM)"");
		SendMessage(GetDlgItem(form_base->hDlg,IDC_CONTTENT),MSG_SETTEXT,0,(LPARAM)WORD_DOWNLOAD);
		Screen.setCurrent(form_base_priv.name);
	} else {
		ShowWindow(Form,SW_SHOWNORMAL);
		form_base = formBaseCreate(&form_base_priv);
		return CreateMyWindowIndirectParam(form_base->priv->dlgInitParam,
				hMainWnd, form_base->priv->dlgProc, 0);
	}

	return 0;
}
