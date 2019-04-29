/*
 * =============================================================================
 *
 *       Filename:  FormMain.c
 *
 *    Description:  主窗口
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

#include "externfunc.h"
#include "debug.h"
#include "screen.h"
#include "config.h"
#include "thread_helper.h"

#include "my_button.h"
#include "my_status.h"
#include "my_static.h"

#include "language.h"
#include "form_base.h"
/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
extern void formSettingLoadBmp(void);

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static int formMainProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam);
static void initPara(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
static void formMainTimerProc1s(HWND hwnd);

static void buttonRecordPress(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonCapturePress(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonVideoPress(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonSettingPress(HWND hwnd, int id, int nc, DWORD add_data);

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#if DBG_FORM_MAIN > 0
	#define DBG_P( x... ) printf( x )
#else
	#define DBG_P( x... )
#endif

enum {
	MSG_MAIN_TIMER_START = MSG_USER + 1,
	MSG_MAIN_TIMER_STOP,
	MSG_MAIN_SHOW_NORMAL,
	MSG_MAIN_LOAD_BMP,
};

typedef void (*InitBmpFunc)(void) ;

#define BMP_LOCAL_PATH "main/"

#define TIME_1S (10 * 5)
#define TIME_100MS (TIME_1S / 10)

enum {
    IDC_TIMER_1S ,	// 1s定时器
    IDC_TIMER_NUM,
};

enum {
    IDC_MYSTATUS_WIFI = IDC_FORM_MAIN_START,
    IDC_MYSTATUS_SDCARD,
    IDC_MYSTATUS_BATTERY,

    IDC_MYSTATIC_DATE,

    IDC_BUTTON_RECORD,
    IDC_BUTTON_CAPTURE,
    IDC_BUTTON_CALL,
    IDC_BUTTON_VIDEO,
    IDC_BUTTON_SETTING,

    IDC_STATE_COM,

};
typedef struct _FormMainTimers {
    void (*proc)(HWND hWnd);
    int time;
}FormMainTimers;

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
PLOGFONT font22;
PLOGFONT font20;
static int bmp_load_finished = 0;
static BmpLocation base_bmps[] = {
	{NULL},
};

static FontLocation font_load[] = {
    {&font22,   22,FONT_WEIGHT_DEMIBOLD},
    {&font20,   20,FONT_WEIGHT_DEMIBOLD},
    {NULL}
};


static MyCtrlStatus ctrls_status[] = {
    {IDC_MYSTATUS_WIFI,   "wifi",16,10,5},
	{IDC_MYSTATUS_SDCARD, "sdcard",54,8,1},
	{IDC_MYSTATUS_BATTERY,"Battery",963,10,1},
	{0},
};
static MyCtrlStatic ctrls_static[] = {
    {IDC_MYSTATIC_DATE,  MYSTATIC_TYPE_TEXT,0,0,1024,40,"",0xffffff,0x00000060},
	{0},
};

static MyCtrlButton ctrls_button[] = {
	{IDC_BUTTON_RECORD,	MYBUTTON_TYPE_TWO_STATE,"记录",80,451,buttonRecordPress,word[WORD_RECORD].string},
	{IDC_BUTTON_CAPTURE,MYBUTTON_TYPE_TWO_STATE,"抓拍",338,451,buttonCapturePress,word[WORD_CAPTURE].string},
	{IDC_BUTTON_VIDEO,	MYBUTTON_TYPE_TWO_STATE,"录像",597,451,buttonVideoPress,word[WORD_VIDEO].string},
	{IDC_BUTTON_SETTING,MYBUTTON_TYPE_TWO_STATE,"设置",855,451,buttonSettingPress,word[WORD_SETTING].string},
	{0},
};


static HWND hwnd_main = HWND_INVALID;
static FormMainTimers timers_tbl[] = {
    {formMainTimerProc1s,               10},
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
	.name = "Fmain",
	.dlgProc = formMainProc,
	.dlgInitParam = &DlgInitParam,
	.initPara =  initPara,
};

static FormBase* form_base = NULL;
/* ---------------------------------------------------------------------------*/
/**
 * @brief formMainTimerStart 开启定时器 单位100ms
 *
 * @param idc_timer 定时器id号，同时也是编号
 */
/* ---------------------------------------------------------------------------*/
static void formMainTimerStart(int idc_timer)
{
	SendMessage(hwnd_main, MSG_MAIN_TIMER_START, (WPARAM)idc_timer, 0);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief formMainTimerStop 关闭定时器
 *
 * @param idc_timer 定时器id号
 */
/* ---------------------------------------------------------------------------*/
static void formMainTimerStop(int idc_timer)
{
	SendMessage(hwnd_main, MSG_MAIN_TIMER_STOP, (WPARAM)idc_timer, 0);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief formMainTimerGetState 返回定时器当前是否激活
 *
 * @param idc_timer
 *
 * @returns 1激活 0未激活
 */
/* ---------------------------------------------------------------------------*/
static int formMainTimerGetState(int idc_timer)
{
	return	IsTimerInstalled(hwnd_main,idc_timer);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief formMainTimerProc1s 窗口相关定时函数
 *
 * @returns 1按键超时退出 0未超时退出
 */
/* ---------------------------------------------------------------------------*/
static void formMainTimerProc1s(HWND hwnd)
{
    static int count = 0;
    char buf[16] = {0};
	static int level_old = 0;
	// 更新网络状态-----
	int level = net_detect();
	if (level != level_old) {
		level_old = level;
		SendMessage(GetDlgItem (hwnd, IDC_MYSTATUS_WIFI),
				MSG_MYSTATUS_SET_LEVEL,level,0);
	}
    sprintf(buf,"%d",count++);
    SendMessage(GetDlgItem (hwnd, IDC_MYSTATIC_DATE),
            MSG_MYSTATIC_SET_TITLE,(WPARAM)buf,0);

}

static void buttonRecordPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
}
static void buttonCapturePress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
}
static void buttonVideoPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
}
static void buttonSettingPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
    createFormSetting(hwnd_main);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief formMainLoadBmp 加载主界面图片
 */
/* ---------------------------------------------------------------------------*/
void formMainLoadBmp(void)
{
    if (bmp_load_finished == 1)
        return;
	printf("[%s]\n", __FUNCTION__);
    bmpsLoad(base_bmps);
    my_button->bmpsLoad(ctrls_button,BMP_LOCAL_PATH);
    my_status->bmpsLoad(ctrls_status,BMP_LOCAL_PATH);
    bmp_load_finished = 1;
}
static void formMainReleaseBmp(void)
{
	printf("[%s]\n", __FUNCTION__);
	bmpsRelease(base_bmps);
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
        ctrls_static[i].font = font20;
        createMyStatic(hDlg,&ctrls_static[i]);
	}
	for (i=0; ctrls_button[i].idc != 0; i++) {
        ctrls_button[i].font = font22;
        createMyButton(hDlg,&ctrls_button[i]);
	}
	for (i=0; ctrls_status[i].idc != 0; i++) {
        createMyStatus(hDlg,&ctrls_status[i]);
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief formMainProc 窗口回调函数
 *
 * @param hDlg
 * @param message
 * @param wParam
 * @param lParam
 *
 * @return
 */
/* ---------------------------------------------------------------------------*/
static int formMainProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case MSG_INITDIALOG:
			{
				hwnd_main = Screen.hMainWnd = hDlg;
                // HDC hdc = GetClientDC (hDlg);
                // mlsSetSlaveScreenInfo(hdc,MLS_INFOMASK_ALL,1,0,
                        // MLS_BLENDMODE_COLORKEY,0x00000100,0x00,3);
                // mlsEnableSlaveScreen(hdc,1);
			} break;

		case MSG_ERASEBKGND:
			{
				drawBackground(hDlg,
						   (HDC)wParam,
						   (const RECT*)lParam,NULL,0x00000100);
			} return 0;


		case MSG_TIMER:
			{
				if ((wParam >= IDC_TIMER_1S) && (wParam < IDC_TIMER_NUM)) {
					timers_tbl[wParam].proc(hDlg);
				}
			} break;

		case MSG_LBUTTONDOWN:
			{
                // if (Public.LCDLight == 0) {
                    // screensaverStart(LCD_ON);
                    // return;
                // }
			} break;

		default:
			break;
	}
	if (form_base->baseProc(form_base,hDlg, message, wParam, lParam) == FORM_STOP)
		return 0;

    return DefaultDialogProc(hDlg, message, wParam, lParam);
}



int createFormMain(HWND hMainWnd)
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
