/*
 * =============================================================================
 *
 *       Filename:  form_setting_Update.c
 *
 *    Description:  Update设置界面
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
#include "my_update.h"

#include "form_base.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static int formSettingUpdateProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
static void initPara(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);

static void buttonNotify(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonUpdateLocal(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonUpdateNetwork(HWND hwnd, int id, int nc, DWORD add_data);

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
	IDC_TIMER_1S = IDC_FORM_SET_UPDATE_STATR,

	IDC_STATIC_TEXT_FIND,
	IDC_STATIC_TEXT_VERSION,
	IDC_STATIC_TEXT_CONTTENT,
	IDC_STATIC_TEXT_NOTFIND,

	IDC_STATIC_IMAGE_UPDATE,
	IDC_STATIC_IMAGE_WARNING,

	IDC_BUTTON_UPDATE_LOCAL,
	IDC_BUTTON_UPDATE_NETWORK,

	IDC_TITLE,
};

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static BITMAP bmp_warning; // 警告
static BITMAP bmp_update; // 升级
static int flag_timer_stop = 0;

static BmpLocation bmp_load[] = {
    {&bmp_warning,	BMP_LOCAL_PATH"ico_警告.png"},
    {&bmp_update,	BMP_LOCAL_PATH"image.png"},
    {NULL},
};

static MY_CTRLDATA ChildCtrls [] = {
    STATIC_IMAGE(103,75l,796,272,IDC_STATIC_IMAGE_UPDATE,(DWORD)&bmp_update),
    STATIC_IMAGE(452,216,120,120,IDC_STATIC_IMAGE_WARNING,(DWORD)&bmp_warning),
    STATIC_LB_LEFT(292,197,220,50,IDC_STATIC_TEXT_FIND,"发现新版本",&font36,0xffffff),
    STATIC_LB_LEFT(292,247,220,50,IDC_STATIC_TEXT_VERSION,"",&font36,0xffffff),
    STATIC_LB_LEFT(361,378,400,150,IDC_STATIC_TEXT_CONTTENT,"",&font20,0xffffff),
    STATIC_LB_LEFT(467,358,100,30,IDC_STATIC_TEXT_NOTFIND,"暂无新版本",&font20,0xffffff),
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
	.name = "FsetUpdate",
	.idc_timer = IDC_TIMER_1S,
	.dlgProc = formSettingUpdateProc,
	.dlgInitParam = &DlgInitParam,
	.initPara =  initPara,
	.auto_close_time_set = 30,
};

static MyCtrlTitle ctrls_title[] = {
	{
        IDC_TITLE,
        MYTITLE_LEFT_EXIT,
        MYTITLE_RIGHT_NULL,
        0,0,1024,40,
        "软件/固件/单片机版本",
        "",
        0xffffff, 0x333333FF,
        buttonNotify,
    },
	{0},
};
static MyCtrlButton ctrls_button[] = {
	{
		.idc = IDC_BUTTON_UPDATE_LOCAL,	
		.flag = MYBUTTON_TYPE_TWO_STATE|MYBUTTON_TYPE_PRESS_COLOR|MYBUTTON_TYPE_TEXT_CENTER,
		.img_name = "本地升级",
		.x = 0, .y = 540, .w = 511, .h = 60,
		.notif_proc = buttonUpdateLocal},
	{	
		.idc = IDC_BUTTON_UPDATE_NETWORK, 
		.flag = MYBUTTON_TYPE_TWO_STATE|MYBUTTON_TYPE_PRESS_COLOR|MYBUTTON_TYPE_TEXT_CENTER,
		.img_name ="网络升级",
		.x = 512,.y = 540,.w = 512, .h = 60,
		.notif_proc = buttonUpdateNetwork},
	{0},
};


static FormBase* form_base = NULL;

static void enableAutoClose(void)
{
	Screen.setCurrent(form_base_priv.name);
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
static void buttonUpdateLocal(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
	if (protocol->isNeedToUpdate(NULL,NULL) == UPDATE_TYPE_SDCARD) {
		flag_timer_stop = 1;
		myUpdateStart(UPDATE_TYPE_SDCARD,NULL,0,NULL);
	}
}
static void buttonUpdateNetwork(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
	if (protocol->isNeedToUpdate(NULL,NULL) == UPDATE_TYPE_SERVER) {
		flag_timer_stop = 1;
		myUpdateStart(UPDATE_TYPE_SERVER,NULL,0,NULL);
	}
}
void formSettingUpdateLoadBmp(void)
{
    bmpsLoad(bmp_load);
}
static void updateInfo(HWND hDlg)
{
	char version[16] = {0},content[256] = {0};
	if (protocol->isNeedToUpdate(version,content)) {
		ShowWindow(GetDlgItem(hDlg,IDC_STATIC_IMAGE_WARNING),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_STATIC_TEXT_NOTFIND),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_STATIC_IMAGE_UPDATE),SW_SHOWNORMAL);
		ShowWindow(GetDlgItem(hDlg,IDC_STATIC_TEXT_FIND),SW_SHOWNORMAL);
		ShowWindow(GetDlgItem(hDlg,IDC_STATIC_TEXT_VERSION),SW_SHOWNORMAL);
		ShowWindow(GetDlgItem(hDlg,IDC_STATIC_TEXT_CONTTENT),SW_SHOWNORMAL);
		SendMessage(GetDlgItem(hDlg,IDC_STATIC_TEXT_VERSION),
				MSG_SETTEXT,0,(LPARAM)version);
		SendMessage(GetDlgItem(hDlg,IDC_STATIC_TEXT_CONTTENT),
				MSG_SETTEXT,0,(LPARAM)content);
	} else {
		ShowWindow(GetDlgItem(hDlg,IDC_STATIC_IMAGE_WARNING),SW_SHOWNORMAL);
		ShowWindow(GetDlgItem(hDlg,IDC_STATIC_TEXT_NOTFIND),SW_SHOWNORMAL);
		ShowWindow(GetDlgItem(hDlg,IDC_STATIC_IMAGE_UPDATE),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_STATIC_TEXT_FIND),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_STATIC_TEXT_VERSION),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_STATIC_TEXT_CONTTENT),SW_HIDE);
	}
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
	updateInfo(hDlg);
}

/* ----------------------------------------------------------------*/
/**
 * @brief formSettingUpdateProc 窗口回调函数
 *
 * @param hDlg
 * @param message
 * @param wParam
 * @param lParam
 *
 * @return
 */
/* ----------------------------------------------------------------*/
static int formSettingUpdateProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    switch(message) // 自定义消息
    {
		case MSG_TIMER:
			{
				if (flag_timer_stop)
					return 0;
			} break;

		case MSG_ENABLE_WINDOW:
			enableAutoClose();
			break;
		case MSG_DISABLE_WINDOW:
			flag_timer_stop = 1;
			break;
		default:
            break;
    }
	if (form_base->baseProc(form_base,hDlg, message, wParam, lParam) == FORM_STOP)
		return 0;
    return DefaultDialogProc(hDlg, message, wParam, lParam);
}


int createFormSettingUpdate(HWND hMainWnd,void (*callback)(void))
{
	HWND Form = Screen.Find(form_base_priv.name);
	if(Form) {
		Screen.setCurrent(form_base_priv.name);
		updateInfo(form_base->hDlg);
		ShowWindow(Form,SW_SHOWNORMAL);
	} else {
		form_base_priv.callBack = callback;
		form_base = formBaseCreate(&form_base_priv);
		return CreateMyWindowIndirectParam(form_base->priv->dlgInitParam,
				hMainWnd, form_base->priv->dlgProc, 0);
	}

	return 0;
}

