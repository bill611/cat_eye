/*
 * =============================================================================
 *
 *       Filename:  form_setting_wifi.c
 *
 *    Description:  wifi设置界面
 *
 *        Version:  1.0
 *        Created:  2018-03-01 23:32:41
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
/* ---------------------------------------------------------------------------*
 *                      include head files
 *----------------------------------------------------------------------------*/
#include "externfunc.h"
#include "screen.h"

#include "my_button.h"
#include "my_title.h"
#include "language.h"
#include "config.h"

#include "form_base.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static int formSettingWifiProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
static void initPara(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);

static void buttonNotify(HWND hwnd, int id, int nc, DWORD add_data);

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#if DBG_FORM_SET_LOCAL > 0
	#define DBG_P( x... ) printf( x )
#else
	#define DBG_P( x... )
#endif

#define BMP_LOCAL_PATH "setting/"
enum {
	IDC_TIMER_1S = IDC_FORM_SETTING_WIFI_STATR,
	IDC_STATIC_IMG__WARNING,
	IDC_STATIC_TEXT_WARNING,

	IDC_TITLE,
};


/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static BITMAP bmp_warning; // 警告


static BmpLocation bmp_load[] = {
    {&bmp_warning,BMP_LOCAL_PATH"ico_警告.png"},
    {NULL},
};

static MY_CTRLDATA ChildCtrls [] = {
    STATIC_IMAGE(452,216,120,120,IDC_STATIC_IMG__WARNING,(DWORD)&bmp_warning),
    STATIC_LB(0,358,1024,25,IDC_STATIC_TEXT_WARNING,word[WORD_WIFI_CLOSED].string,&font20,0xffffff),
};

static MY_DLGTEMPLATE DlgInitParam =
{
    WS_NONE,
    // WS_EX_AUTOSECONDARYDC,
    WS_EX_NONE,
    0,0,SCR_WIDTH,SCR_HEIGHT,
    "",
    0, 0,       //menu and icon is null
    sizeof(ChildCtrls)/sizeof(MY_CTRLDATA),
    ChildCtrls, //pointer to control array
    0           //additional data,must be zero
};

static FormBasePriv form_base_priv= {
	.name = "Fsetwifi",
	.idc_timer = IDC_TIMER_1S,
	.dlgProc = formSettingWifiProc,
	.dlgInitParam = &DlgInitParam,
	.initPara =  initPara,
};

static MyCtrlButton ctrls_button[] = {
	{0},
};

static MyCtrlTitle ctrls_title[] = {
	{
        IDC_TITLE,
        MYTITLE_LEFT_EXIT,
        MYTITLE_RIGHT_SWICH,
        0,0,1024,40,
        word[WORD_WIFI_SET].string,
        "",
        0xffffff, 0x333333FF,
        buttonNotify,
    },
	{0},
};

static FormBase* form_base = NULL;

static void showWarning(HWND hwnd,int on_off)
{
    if (on_off) {
        ShowWindow(GetDlgItem(hwnd,IDC_STATIC_IMG__WARNING),SW_HIDE);
        ShowWindow(GetDlgItem(hwnd,IDC_STATIC_TEXT_WARNING),SW_HIDE);
    } else {
        ShowWindow(GetDlgItem(hwnd,IDC_STATIC_IMG__WARNING),SW_SHOWNORMAL);
        ShowWindow(GetDlgItem(hwnd,IDC_STATIC_TEXT_WARNING),SW_SHOWNORMAL);
    }
}
/* ----------------------------------------------------------------*/
/**
 * @brief buttonNotify 退出按钮
 *
 * @param hwnd
 * @param id
 * @param nc
 * @param add_data
 */
/* ----------------------------------------------------------------*/
static void buttonNotify(HWND hwnd, int id, int nc, DWORD add_data)
{
    if (nc == MYTITLE_BUTTON_EXIT)
        ShowWindow(GetParent(hwnd),SW_HIDE);
    else if (nc == MYTITLE_BUTTON_SWICH) {
        g_config.net_config.enable = add_data;
        showWarning(GetParent(hwnd),add_data);
    }
}

void formSettingWifiLoadBmp(void)
{
    bmpsLoad(bmp_load);
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
	int i;
    for (i=0; ctrls_title[i].idc != 0; i++) {
        ctrls_title[i].font = font20;
        createMyTitle(hDlg,&ctrls_title[i]);
    }
    for (i=0; ctrls_button[i].idc != 0; i++) {
        ctrls_button[i].font = font22;
        createMyButton(hDlg,&ctrls_button[i]);
    }
	SendMessage(GetDlgItem(hDlg,IDC_TITLE),
            MSG_MYTITLE_SET_SWICH, (WPARAM)g_config.net_config.enable, 0);
    showWarning(hDlg,g_config.net_config.enable);
}

/* ----------------------------------------------------------------*/
/**
 * @brief formSettingWifiProc 窗口回调函数
 *
 * @param hDlg
 * @param message
 * @param wParam
 * @param lParam
 *
 * @return
 */
/* ----------------------------------------------------------------*/
static int formSettingWifiProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    switch(message) // 自定义消息
    {
        default:
            break;
    }
	if (form_base->baseProc(form_base,hDlg, message, wParam, lParam) == FORM_STOP)
		return 0;
    return DefaultDialogProc(hDlg, message, wParam, lParam);
}

int createFormSettingWifi(HWND hMainWnd,void (*callback)(void))
{
	HWND Form = Screen.Find(form_base_priv.name);
	if(Form) {
		ShowWindow(Form,SW_SHOWNORMAL);
	} else {
		form_base_priv.hwnd = hMainWnd;
		form_base_priv.callBack = callback;
		form_base = formBaseCreate(&form_base_priv);
		return CreateMyWindowIndirectParam(form_base->priv->dlgInitParam,
				form_base->priv->hwnd,
				form_base->priv->dlgProc, 0);
	}

	return 0;
}

