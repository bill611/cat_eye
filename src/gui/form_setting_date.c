/*
 * =============================================================================
 *
 *       Filename:  form_setting_Date.c
 *
 *    Description:  日期设置界面
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
#include "config.h"
#include "my_audio.h"

#include "my_button.h"
#include "my_scroll.h"
#include "my_title.h"

#include "form_base.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static int formSettingDateProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
static void initPara(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);

static void buttonExitPress(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonTopPress(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonBottomPress(HWND hwnd, int id, int nc, DWORD add_data);

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
	IDC_TIMER_1S = IDC_FORM_SETTING_TIEMR,
	IDC_BUTTON_EXIT,
	IDC_BUTTON_TITLE,
	IDC_MYSCROLL_YEAR,
	IDC_MYSCROLL_MONTH,
	IDC_MYSCROLL_DATE,
	IDC_TITLE,
};


struct ScrollviewItem {
	char title[32]; // 左边标题
	char text[32];  // 右边文字
	int (*callback)(HWND,void (*callback)(void)); // 点击回调函数
	int index;  // 元素位置
};

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static int bmp_load_finished = 0;
static int flag_Date_stop = 0;

static BmpLocation bmp_load[] = {
    {NULL},
};

static MY_CTRLDATA ChildCtrls [] = {
};

static MyCtrlButton ctrls_button[] = {
	{0},
};

static MyCtrlScroll ctrls_scroll[] = {
	{IDC_MYSCROLL_YEAR,	0,"年",2019, 2050,	153,40,180,560},
	{IDC_MYSCROLL_MONTH,0,"月",1, 12,		423,40,180,560},
	{IDC_MYSCROLL_DATE,	0,"日",1, 31,		692,40,180,560},
	{0},
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
	.name = "FsetDate",
	.idc_timer = IDC_TIMER_1S,
	.dlgProc = formSettingDateProc,
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
        "日期设置",
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
	flag_Date_stop = 0;	
}
/* ----------------------------------------------------------------*/
/**
 * @brief buttonTopPress wifi设置
 *
 * @param hwnd
 * @param id
 * @param nc
 * @param add_data
 */
/* ----------------------------------------------------------------*/
static void buttonTopPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
	// if (g_config.ring_num > 0) {
		// g_config.ring_num--;
		// myAudioPlayRingOnce();
		// ConfigSavePublic();
	// }
}

static void buttonBottomPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
	// if (g_config.ring_num < (MAX_RINGS_NUM - 1)) {
		// g_config.ring_num++;
	// }
}

static void reloadDate(void)
{
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

void formSettingDateLoadBmp(void)
{
    if (bmp_load_finished == 1)
        return;

	printf("[%s]\n", __FUNCTION__);
    bmpsLoad(bmp_load);
	my_scroll->bmpsLoad(ctrls_scroll,BMP_LOCAL_PATH);
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
    for (i=0; ctrls_scroll[i].idc != 0; i++) {
        ctrls_scroll[i].font = font22;
        createMyScroll(hDlg,&ctrls_scroll[i]);
    }
	reloadDate();
}

/* ----------------------------------------------------------------*/
/**
 * @brief formSettingDateProc 窗口回调函数
 *
 * @param hDlg
 * @param message
 * @param wParam
 * @param lParam
 *
 * @return
 */
/* ----------------------------------------------------------------*/
static int formSettingDateProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    switch(message) // 自定义消息
    {
		case MSG_TIMER:
			{
				if (flag_Date_stop)
					return 0;
			} break;

		case MSG_ENABLE_WINDOW:
			enableAutoClose();
			break;
		case MSG_DISABLE_WINDOW:
			flag_Date_stop = 1;
			break;
        default:
            break;
    }
	if (form_base->baseProc(form_base,hDlg, message, wParam, lParam) == FORM_STOP)
		return 0;
    return DefaultDialogProc(hDlg, message, wParam, lParam);
}

int createFormSettingDate(HWND hMainWnd,void (*callback)(void))
{
	HWND Form = Screen.Find(form_base_priv.name);
	if(Form) {
		Screen.setCurrent(form_base_priv.name);
		reloadDate();
		ShowWindow(Form,SW_SHOWNORMAL);
	} else {
		if (bmp_load_finished == 0) {
			return 0;
		}
		form_base_priv.callBack = callback;
		form_base = formBaseCreate(&form_base_priv);
		return CreateMyWindowIndirectParam(form_base->priv->dlgInitParam,
				hMainWnd, form_base->priv->dlgProc, 0);
	}

	return 0;
}

