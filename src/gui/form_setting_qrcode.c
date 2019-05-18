/*
 * =============================================================================
 *
 *       Filename:  form_setting_Qrcode.c
 *
 *    Description:  Qrcode设置界面
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
#include <string.h>
#include "externfunc.h"
#include "screen.h"

#include "my_button.h"
#include "my_title.h"
#include "config.h"
#include "protocol.h"

#include "form_base.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static void buttonGetImei(HWND hwnd, int id, int nc, DWORD add_data);
static int formSettingQrcodeProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
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
	IDC_TIMER_1S = IDC_FORM_QRCODE_STATR,
	IDC_STATIC_TEXT_IMEI,
	IDC_STATIC_TEXT_APP_URL,
	IDC_STATIC_TEXT_HELP,
	IDC_STATIC_TEXT_GETIMEI,

	IDC_STATIC_IMAGE_IMEI,
	IDC_STATIC_IMAGE_APP_URL,

	IDC_TITLE,
	IDC_BUTTON_GETIMEI,
};

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static int flag_timer_stop = 0;
static BITMAP *bmp_imei;
static BITMAP *bmp_app_url;

static BmpLocation bmp_load[] = {
    {NULL},
};

static MY_CTRLDATA ChildCtrls [] = {
    STATIC_LB(170,384,220,40,IDC_STATIC_TEXT_IMEI,"设备二维码",&font22,0xffffff),
    STATIC_LB(633,384,220,40,IDC_STATIC_TEXT_APP_URL,"APP下载二维码",&font22,0xffffff),
    STATIC_LB(0,537,1024,40,IDC_STATIC_TEXT_HELP,"下载手机APP扫码设备二维码,添加智能猫眼设备",&font20,0xffffff),
    STATIC_LB(0,497,1024,40,IDC_STATIC_TEXT_GETIMEI,"",&font20,0xffffff),

    STATIC_IMAGE(170,138,220,220,IDC_STATIC_IMAGE_IMEI,0),
    STATIC_IMAGE(633,138,220,220,IDC_STATIC_IMAGE_APP_URL,0),
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
	.name = "FsetQrcode",
	.idc_timer = IDC_TIMER_1S,
	.dlgProc = formSettingQrcodeProc,
	.dlgInitParam = &DlgInitParam,
	.initPara =  initPara,
};

static MyCtrlTitle ctrls_title[] = {
	{
        IDC_TITLE,
        MYTITLE_LEFT_EXIT,
        MYTITLE_RIGHT_NULL,
        0,0,1024,40,
        "二维码",
        "",
        0xffffff, 0x333333FF,
        buttonNotify,
    },
	{0},
};

static MyCtrlButton ctrls_button[] = {
	{IDC_BUTTON_GETIMEI,MYBUTTON_TYPE_TWO_STATE | MYBUTTON_TYPE_TEXT_CENTER,"abc",170,138,buttonGetImei},
	{0},
};
static FormBase* form_base = NULL;

static void getImeiCallback(int result)
{
	if (result) {
		SendMessage(GetDlgItem(form_base->hDlg,IDC_STATIC_TEXT_GETIMEI),
				MSG_SETTEXT,0,(LPARAM)"机身码获取成功!");
	} else {
		SendMessage(GetDlgItem(form_base->hDlg,IDC_STATIC_TEXT_GETIMEI),
				MSG_SETTEXT,0,(LPARAM)"机身码获取失败!");
	}
}
static void buttonGetImei(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
	flag_timer_stop = 1;
	pro_hardcloud->getImei(getImeiCallback);
	SendMessage(GetDlgItem(GetParent(hwnd),IDC_STATIC_TEXT_GETIMEI),
			MSG_SETTEXT,0,(LPARAM)"正在获取机身码，请稍后...");
}

static void enableAutoClose(void)
{
	flag_timer_stop = 0;
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
}
void formSettingQrcodeLoadBmp(void)
{
    bmpsLoad(bmp_load);
    my_button->bmpsLoad(ctrls_button,BMP_LOCAL_PATH);
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
}

/* ----------------------------------------------------------------*/
/**
 * @brief formSettingQrcodeProc 窗口回调函数
 *
 * @param hDlg
 * @param message
 * @param wParam
 * @param lParam
 *
 * @return
 */
/* ----------------------------------------------------------------*/
static int formSettingQrcodeProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    switch(message) // 自定义消息
    {
		case MSG_TIMER:
			{
				if (flag_timer_stop)
					return 0;
			} break;

		case MSG_SHOWWINDOW:
			{
				if (wParam == SW_HIDE) {
					if (bmp_app_url) {
						UnloadBitmap(bmp_app_url);
						bmp_app_url = NULL;
					}
					if (bmp_imei) {
						UnloadBitmap(bmp_imei);
						bmp_imei = NULL;
					}
				}
			} break;

		default:
            break;
    }
	if (form_base->baseProc(form_base,hDlg, message, wParam, lParam) == FORM_STOP)
		return 0;
    return DefaultDialogProc(hDlg, message, wParam, lParam);
}


int createFormSettingQrcode(HWND hMainWnd,void (*callback)(void))
{
	HWND Form = Screen.Find(form_base_priv.name);
	if (fileexists(QRCODE_APP)) {
		bmp_app_url = (BITMAP *) calloc(1,sizeof(BITMAP));
		bmpLoad(bmp_app_url,QRCODE_APP);
	}
	if (fileexists(QRCODE_IMIE)) {
		bmp_imei = (BITMAP *) calloc(1,sizeof(BITMAP));
		bmpLoad(bmp_imei,QRCODE_IMIE);
	}

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

