/*
 * =============================================================================
 *
 *       Filename:  form_setting.c
 *
 *    Description:  设置界面
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

#include "form_base.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
extern int createFormSettingWifi(HWND hMainWnd,void (*callback)(void));
extern int createFormSettingLocoal(HWND hMainWnd,void (*callback)(void));

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static int formSettingProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
static void initPara(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);

static void buttonExitPress(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonWifiPress(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonScreenPress(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonDoorBellPress(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonTimerPress(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonMutePress(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonAlarmPress(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonFactoryPress(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonLocalPress(HWND hwnd, int id, int nc, DWORD add_data);

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
	IDC_TIMER_1S = IDC_FORM_SETTING_START,
	IDC_BUTTON_EXIT,
	IDC_BUTTON_WIFI,
	IDC_BUTTON_SCREEN,
	IDC_BUTTON_DOORBELL,
	IDC_BUTTON_TIMER,
	IDC_BUTTON_MUTE,
	IDC_BUTTON_ALARM,
	IDC_BUTTON_FACTORY,
	IDC_BUTTON_LOCAL,

	IDC_TITLE,
};


/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static BITMAP bmp_bkg_setting; // 背景

static int bmp_load_finished = 0;
static int flag_timer_stop = 0;

static BmpLocation bmp_load[] = {
    // {&bmp_bkg_setting,BMP_LOCAL_PATH"bkg_setting.png"},
    {NULL},
};

static MY_CTRLDATA ChildCtrls [] = {
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
	.name = "Fset",
	.idc_timer = IDC_TIMER_1S,
	.dlgProc = formSettingProc,
	.dlgInitParam = &DlgInitParam,
	.initPara =  initPara,
};

static MyCtrlButton ctrls_button[] = {
	{IDC_BUTTON_WIFI,	 MYBUTTON_TYPE_TWO_STATE,"wifi设置",99,	129,buttonWifiPress},
	{IDC_BUTTON_SCREEN,	 MYBUTTON_TYPE_TWO_STATE,"屏幕设置",338,129,buttonScreenPress},
	{IDC_BUTTON_DOORBELL,MYBUTTON_TYPE_TWO_STATE,"门铃设置",577,129,buttonDoorBellPress},
	{IDC_BUTTON_TIMER,	 MYBUTTON_TYPE_TWO_STATE,"时间设置",817,129,buttonTimerPress},
	{IDC_BUTTON_MUTE,	 MYBUTTON_TYPE_TWO_STATE,"免扰设置",99,	366,buttonMutePress},
	{IDC_BUTTON_ALARM,	 MYBUTTON_TYPE_TWO_STATE,"报警设置",338,366,buttonAlarmPress},
	{IDC_BUTTON_FACTORY, MYBUTTON_TYPE_TWO_STATE,"恢复出厂",577,366,buttonFactoryPress},
	{IDC_BUTTON_LOCAL,	 MYBUTTON_TYPE_TWO_STATE,"本机设置",817,366,buttonLocalPress},
	{0},
};
static MyCtrlTitle ctrls_title[] = {
	{
        IDC_TITLE, 
        MYTITLE_LEFT_EXIT,
        MYTITLE_RIGHT_NULL,
        0,0,1024,40,
        "设置",
        "",
        0xffffff, 0x333333FF,
        buttonExitPress,
    },
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
 * @brief buttonWifiPress wifi设置
 *
 * @param hwnd
 * @param id
 * @param nc
 * @param add_data
 */
/* ----------------------------------------------------------------*/
static void buttonWifiPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
	flag_timer_stop = 1;
    createFormSettingWifi(GetParent(hwnd),enableAutoClose);
}
static void buttonScreenPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
}
static void buttonDoorBellPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
}
static void buttonTimerPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
}
static void buttonMutePress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
}
static void buttonAlarmPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
}
static void buttonFactoryPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
}
static void buttonLocalPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
	flag_timer_stop = 1;
    createFormSettingLocoal(GetParent(hwnd),enableAutoClose);
}

/* ----------------------------------------------------------------*/
/**
 * @brief buttonExitPress 退出按钮
 *
 * @param hwnd
 * @param id
 * @param nc
 * @param add_data
 */
/* ----------------------------------------------------------------*/
static void buttonExitPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	ShowWindow(GetParent(hwnd),SW_HIDE);
}

void formSettingLoadBmp(void)
{
    if (bmp_load_finished == 1)
        return;

	printf("[%s]\n", __FUNCTION__);
    bmpsLoad(bmp_load);
    my_button->bmpsLoad(ctrls_button,BMP_LOCAL_PATH);	
    bmp_load_finished = 1;
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
 * @brief formSettingProc 窗口回调函数
 *
 * @param hDlg
 * @param message
 * @param wParam
 * @param lParam
 *
 * @return
 */
/* ----------------------------------------------------------------*/
static int formSettingProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
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

int createFormSetting(HWND hMainWnd,void (*callback)(void))
{
	HWND Form = Screen.Find(form_base_priv.name);
	if(Form) {
		Screen.setCurrent(form_base_priv.name);
		ShowWindow(Form,SW_SHOWNORMAL);
	} else {
        if (bmp_load_finished == 0) {
            // topMessage(hMainWnd,TOPBOX_ICON_LOADING,NULL );
            return 0;
        }
		form_base_priv.callBack = callback;
		form_base = formBaseCreate(&form_base_priv);
		return CreateMyWindowIndirectParam(form_base->priv->dlgInitParam,
				hMainWnd, form_base->priv->dlgProc, 0);
	}

	return 0;
}

