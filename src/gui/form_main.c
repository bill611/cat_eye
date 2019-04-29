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
#include "form_main.h"
/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
extern void formSettingLoadBmp(void);

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static void formMainTimerProc1s(void);

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
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
PLOGFONT font22;
PLOGFONT font20;
static BITMAP 	bkg;
static BmpLocation base_bmps[] = {
	{&bkg,BMP_LOCAL_PATH"bg_1.png"},
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
    {IDC_MYSTATIC_DATE,  MYSTATIC_TYPE_TEXT,0,0,1024,40,"",0xffffff,0x33333380},
	{0},
};

static MyCtrlButton ctrls_button[] = {
	{IDC_BUTTON_RECORD,	MYBUTTON_TYPE_TWO_STATE,"记录",80,451,buttonRecordPress,word[WORD_RECORD].string},
	{IDC_BUTTON_CAPTURE,MYBUTTON_TYPE_TWO_STATE,"抓拍",338,451,buttonCapturePress,word[WORD_CAPTURE].string},
	{IDC_BUTTON_VIDEO,	MYBUTTON_TYPE_TWO_STATE,"录像",597,451,buttonVideoPress,word[WORD_VIDEO].string},
	{IDC_BUTTON_SETTING,MYBUTTON_TYPE_TWO_STATE,"设置",855,451,buttonSettingPress,word[WORD_SETTING].string},
	{0},
};

static InitBmpFunc load_bmps_func[] = {
    formSettingLoadBmp,
	NULL,
};

static HWND hwnd_main = HWND_INVALID;
static FormMainTimers timers_tbl[] = {
    {formMainTimerProc1s,               10},
};

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
static void formMainTimerProc1s(void)
{
	static int level_old = 0;
	// 更新网络状态-----
	int level = net_detect();
	if (level != level_old) {
		level_old = level;
		SendMessage(GetDlgItem (hwnd_main, IDC_MYSTATUS_WIFI),
				MSG_MYSTATUS_SET_LEVEL,level,0);
	}

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

static void showNormal(HWND hWnd)
{
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief formMainUpdateMute 更新静音状态
 */
/* ---------------------------------------------------------------------------*/
void formMainUpdateMute(HWND hWnd)
{
	// 更新静音状态
}


/* ---------------------------------------------------------------------------*/
/**
 * @brief formMainCreateControl 创建控件
 *
 * @param hWnd
 */
/* ---------------------------------------------------------------------------*/
static void formMainCreateControl(HWND hWnd)
{
	int i;
	for (i=0; ctrls_static[i].idc != 0; i++) {
        ctrls_static[i].font = font20;
        createMyStatic(hWnd,&ctrls_static[i]);
	}
	for (i=0; ctrls_button[i].idc != 0; i++) {
        ctrls_button[i].font = font22;
        createMyButton(hWnd,&ctrls_button[i]);
	}
	for (i=0; ctrls_status[i].idc != 0; i++) {
        createMyStatus(hWnd,&ctrls_status[i]);
	}
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief formMainLoadBmp 加载主界面图片
 */
/* ---------------------------------------------------------------------------*/
static void formMainLoadBmp(void)
{
	printf("[%s]\n", __FUNCTION__);
    bmpsLoad(base_bmps);
    my_button->bmpsLoad(ctrls_button,BMP_LOCAL_PATH);
    my_status->bmpsLoad(ctrls_status,BMP_LOCAL_PATH);
}
static void formMainReleaseBmp(void)
{
	printf("[%s]\n", __FUNCTION__);
	bmpsRelease(base_bmps);
}

static void * loadBmpsThread(void *arg)
{
    int i;
    // char *cmd = arg;
    for (i=0; load_bmps_func[i] != NULL; i++) {
        load_bmps_func[i]();
    }
    return NULL;
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
static int formMainProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case MSG_CREATE:
			{
				screenInit();
				Screen.Add(hWnd,"TFrmMain");
				hwnd_main = Screen.hMainWnd = hWnd;
				// 装载所有图片
				formMainLoadBmp();
				createThread(loadBmpsThread,"1");
                fontsLoad(font_load);
				formMainCreateControl(hWnd);
                formMainTimerStart(IDC_TIMER_1S);
				// screensaverStart(LCD_ON);
			} break;

		case MSG_ERASEBKGND:
			{
				drawBackground(hWnd,
						   (HDC)wParam,
						   (const RECT*)lParam,&bkg);
			} return 0;

		case MSG_MAIN_SHOW_NORMAL:
			{
                Screen.ReturnMain();
                showNormal(hWnd);
			} return 0;

		case MSG_MAIN_LOAD_BMP:
			{
                createThread(loadBmpsThread,NULL);
			} return 0;

		case MSG_MAIN_TIMER_START:
			{
				SetTimer(hWnd, wParam,
						timers_tbl[wParam].time * TIME_100MS);
			} return 0;

		case MSG_MAIN_TIMER_STOP:
			{
				if (IsTimerInstalled(hWnd,wParam) == TRUE) {
					KillTimer (hwnd_main,wParam);
				}
			} return 0;

		case MSG_TIMER:
			{
				if ((wParam >= IDC_TIMER_1S) && (wParam < IDC_TIMER_NUM)) {
					timers_tbl[wParam].proc();
				}
			} return 0;

		case MSG_LBUTTONDOWN:
			{
                // if (Public.LCDLight == 0) {
                    // screensaverStart(LCD_ON);
                    // return;
                // }
			} break;

		case MSG_UPDATESTATUS:
			{
				formMainUpdateMute(hWnd);
			} break;

		case MSG_DESTROY:
			{
				Screen.Del(hWnd);
				DestroyAllControls (hWnd);
			} return 0;

		case MSG_CLOSE:
			{
				int i;
				for (i=IDC_TIMER_1S; i<IDC_TIMER_NUM; i++) {
					formMainTimerStop(i);
				}
				DestroyMainWindow (hWnd);
				PostQuitMessage (hWnd);
			} return 0;

		default:
			break;
	}
	return DefaultMainWinProc(hWnd, message, wParam, lParam);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief formMainLoop 主窗口消息循环
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int formMainLoop(void)
{
    MSG Msg;
	while (GetMessage(&Msg, hwnd_main)) {
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}
    my_button->bmpsRelease(ctrls_button);
    my_status->bmpsRelease(ctrls_status);
    my_button->unregist();
    my_status->unregist();
    my_static->unregist();

    MainWindowThreadCleanup (hwnd_main);
	hwnd_main = Screen.hMainWnd = 0;
	formMainReleaseBmp();
	return 0;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief formMainCreate 创建主窗口
 *
 * @param AppProc
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
FormMain * formMainCreate(void)
{
    MAINWINCREATE CreateInfo;

    initMyButton();
    initMyStatus();
    initMyStatic();
    my_button->regist();
    my_status->regist();
    my_static->regist();

    CreateInfo.dwStyle = WS_NONE;
	CreateInfo.dwExStyle = WS_EX_AUTOSECONDARYDC;
	// CreateInfo.dwExStyle = WS_EX_NONE;
    CreateInfo.spCaption = "cateye";
    CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor(IDC_ARROW);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = formMainProc;
    CreateInfo.lx = 0;
    CreateInfo.ty = 0;
    CreateInfo.rx = SCR_WIDTH;
    CreateInfo.by = SCR_HEIGHT;
    CreateInfo.iBkColor = GetWindowElementColor(WE_MAINC_THREED_BODY);
    CreateInfo.dwAddData = 0;
    CreateInfo.dwReserved = 0;
    CreateInfo.hHosting = HWND_DESKTOP;

	CreateMainWindow (&CreateInfo);
	if (hwnd_main == HWND_INVALID)
		return NULL;
	ShowWindow(hwnd_main, SW_SHOWNORMAL);

	FormMain * this = (FormMain *) calloc (1,sizeof(FormMain));
	this->loop = formMainLoop;
	this->timerStart = formMainTimerStart;
	this->timerStop = formMainTimerStop;
	this->timerGetState = formMainTimerGetState;

	return this;
}

