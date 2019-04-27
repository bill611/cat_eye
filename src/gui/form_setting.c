/*
 * =============================================================================
 *
 *       Filename:  form_setting.c
 *
 *    Description:  输入密码界面
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
#include "my_static.h"
#include "language.h"

#include "form_base.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

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
	IDC_BUTTON_EXIT = IDC_FORM_SETTING_START,
	IDC_BUTTON_WIFI,
	IDC_BUTTON_SCREEN,
	IDC_BUTTON_DOORBELL,
	IDC_BUTTON_TIMER,
	IDC_BUTTON_MUTE,
	IDC_BUTTON_ALARM,
	IDC_BUTTON_FACTORY,
	IDC_BUTTON_LOCAL,

	IDC_STATIC_TITLE,
	IDC_STATIC_TITLE_BKG,
};


/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static BITMAP bmp_bkg_setting; // 背景

static int bmp_load_finished = 0;

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
	.dlgProc = formSettingProc,
	.dlgInitParam = &DlgInitParam,
	.initPara =  initPara,
};

static MyCtrlButton ctrls_button[] = {
	{IDC_BUTTON_WIFI,	 MYBUTTON_TYPE_TWO_STATE,"wifi设置",99,	129,buttonWifiPress,word[WORD_WIFI_SET].string},
	{IDC_BUTTON_SCREEN,	 MYBUTTON_TYPE_TWO_STATE,"屏幕设置",338,129,buttonScreenPress,word[WORD_SCREEN_SET].string},
	{IDC_BUTTON_DOORBELL,MYBUTTON_TYPE_TWO_STATE,"门铃设置",577,129,buttonDoorBellPress,word[WORD_DOORBELL_SET].string},
	{IDC_BUTTON_TIMER,	 MYBUTTON_TYPE_TWO_STATE,"时间设置",817,129,buttonTimerPress,word[WORD_TIMER_SET].string},
	{IDC_BUTTON_MUTE,	 MYBUTTON_TYPE_TWO_STATE,"免扰设置",99,	366,buttonMutePress,word[WORD_MUTE_SET].string},
	{IDC_BUTTON_ALARM,	 MYBUTTON_TYPE_TWO_STATE,"报警设置",338,366,buttonAlarmPress,word[WORD_ALARM_SET].string},
	{IDC_BUTTON_FACTORY, MYBUTTON_TYPE_TWO_STATE,"恢复出厂",577,366,buttonFactoryPress,word[WORD_FACTORY].string},
	{IDC_BUTTON_LOCAL,	 MYBUTTON_TYPE_TWO_STATE,"本机设置",817,366,buttonLocalPress,word[WORD_LOCAL_SET].string},
	{IDC_BUTTON_EXIT,	 MYBUTTON_TYPE_ONE_STATE,"arrow-back",16,10,buttonExitPress},
	{0},
};
static MyCtrlStatic ctrls_static[] = {
	{IDC_BUTTON_EXIT, MYSTATIC_TYPE_TEXT,0,0,1024,40,word[WORD_SETTING].string,0xffffff,0x333333},
	{0},
};

static FormBase* form_base = NULL;

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
	if (nc != BN_CLICKED)
		return;
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
    for (i=0; ctrls_static[i].idc != 0; i++) {
        ctrls_static[i].font = font22;
        createMyStatic(hDlg,&ctrls_static[i]);
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
        default:
            break;
    }
	if (form_base->baseProc(form_base,hDlg, message, wParam, lParam) == FORM_STOP)
		return 0;
    return DefaultDialogProc(hDlg, message, wParam, lParam);
}

int createFormSetting(HWND hMainWnd)
{
	HWND Form = Screen.Find(form_base_priv.name);
	if(Form) {
		ShowWindow(Form,SW_SHOWNORMAL);
	} else {
        if (bmp_load_finished == 0) {
            // topMessage(hMainWnd,TOPBOX_ICON_LOADING,NULL );
            return 0;
        }
		form_base_priv.hwnd = hMainWnd;
		form_base = formBaseCreate(&form_base_priv);
		return CreateMyWindowIndirectParam(form_base->priv->dlgInitParam,
				form_base->priv->hwnd,
				form_base->priv->dlgProc, 0);
	}

	return 0;
}

