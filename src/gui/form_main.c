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

#include "form_main.h"
/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
extern int createFormVersion(HWND hMainWnd);

extern void formVersionLoadBmp(void);

extern void formVersionLoadLock(void);
/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static void formMainLoadBmp(void);
static int formMainTimerProc1s(void);

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#if DBG_FORM_MAIN > 0
	#define DBG_P( x... ) printf( x )
#else
	#define DBG_P( x... )
#endif

typedef void (*InitBmpFunc)(void) ;

struct _RegistFunc {
	BOOL (*registControl)(void);
	void (*unregistControl)(void);
};

#define BMP_LOCAL_PATH "main/"

#define TIME_1S (10 * 5)
#define TIME_100MS (TIME_1S / 10)

enum {
    IDC_TIMER_1S ,	// 1s定时器
    IDC_TIMER_NUM,
};

enum {
    IDC_TOOLBAR_WIFI = 100,
    IDC_TOOLBAR_SDCARD,
    IDC_TOOLBAR_DATE,
    IDC_TOOLBAR_BATTERY,

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
static BITMAP bmp_wifi; // wifi

// 注册自定义控件
static struct _RegistFunc regist_my_controls[] = {
	{myButtonRegist,myButtonCleanUp},
	{myStatusRegist,myStatusCleanUp},
	{NULL}
};

static BmpLocation base_bmps[] = {
	{NULL},
};


static MyCtrlStatus ctrls_toolbar[] = {
	{IDC_TOOLBAR_WIFI,	"wifi",10,50,50,50,4},
	{0},
};

static MyCtrlButton ctrls_button[] = {
	{IDC_BUTTON_RECORD,	"record",40,400,95,85},
	{IDC_BUTTON_CAPTURE,"capture",160,400,95,85},
	{IDC_BUTTON_CALL,	"call",280,400,95,85},
	{IDC_BUTTON_VIDEO,	"video",400,400,95,85},
	{IDC_BUTTON_SETTING,"setting",520,400,95,85},
	{0},
};

static InitBmpFunc load_bmps_func[] = {
	NULL,
    // formVersionLoadBmp,
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
static int formMainTimerProc1s(void)
{
    // static int count = 0;
    // if (count >= 4)
        // count = 0;
    // SendMessage(GetDlgItem (hwnd_main, IDC_TOOLBAR_WIFI), MSG_MYSTATU_SET_LEVEL,count++,0);
    // printf("%s\n",__FUNCTION__ );
	return 0;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief formMainSetNetWorkState 改变网口状态图标
 *
 * @param state 0未连接 1连接
 */
/* ---------------------------------------------------------------------------*/
static void formMainSetNetWorkState(int state)
{
	static int state_old = 0;
	if (state_old == state) {
		return;
	}
	state_old = state;
    // if (state)
        // ShowWindow(GetDlgItem (hwnd_main, IDC_WIFI), SW_SHOWNORMAL);
    // else
        // ShowWindow(GetDlgItem (hwnd_main, IDC_WIFI), SW_HIDE);

}

static void optControlsNotify(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
	HWND parent_hwnd = GetParent(hwnd);
    switch (id) {
        // case IDC_SYSTEM:
			// createFormVersion(parent_hwnd);
			   // break;
        default:
            break;
    }
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
	for (i=0; ctrls_button[i].idc != 0; i++)
        createSkinButton(hWnd,&ctrls_button[i], 1, 0);
	for (i=0; ctrls_toolbar[i].idc != 0; i++)
        createMyStatus(hWnd,&ctrls_toolbar[i]);
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
	myButtonBmpsLoad(ctrls_button,BMP_LOCAL_PATH);
    myStatusBmpsLoad(ctrls_toolbar,BMP_LOCAL_PATH);
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
				formMainCreateControl(hWnd);
                formMainTimerStart(IDC_TIMER_1S);
				// screensaverStart(LCD_ON);
			} break;

		case MSG_ERASEBKGND:
			{
				drawBackground(hWnd,
						   (HDC)wParam,
						   (const RECT*)lParam,NULL);
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
				if (IsTimerInstalled(hwnd_main,wParam) == TRUE) {
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
	int i;
	for (i=0; regist_my_controls[i].unregistControl != NULL; i++)
		regist_my_controls[i].unregistControl();	
    MainWindowThreadCleanup (hwnd_main);
	hwnd_main = Screen.hMainWnd = 0;
	bmpsRelease(base_bmps,NELEMENTS(base_bmps));
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
	int i;
	for (i=0; regist_my_controls[i].unregistControl != NULL; i++)
		regist_my_controls[i].registControl();	

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
    this->setNetWorkState = formMainSetNetWorkState;

	return this;
}

