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
/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
extern void createFormMain(HWND hMainWnd);
extern void formMainLoadBmp(void);
extern void formSettingLoadBmp(void);
extern void formVideoLoadBmp(void);

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#if DBG_FORM_MAIN > 0
	#define DBG_P( x... ) printf( x )
#else
	#define DBG_P( x... )
#endif

enum {
	MSG_MAIN_LOAD_BMP = MSG_USER + 1,
};

typedef void (*InitBmpFunc)(void) ;

#define TIME_1S (10 * 5)

enum {
    IDC_TIMER_1S ,	// 1s定时器
    IDC_TIMER_NUM,
};

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
PLOGFONT font22;
PLOGFONT font20;

static FontLocation font_load[] = {
    {&font22,   22,FONT_WEIGHT_DEMIBOLD},
    {&font20,   20,FONT_WEIGHT_DEMIBOLD},
    {NULL}
};


static InitBmpFunc load_bmps_func[] = {
    formMainLoadBmp,
    formSettingLoadBmp,
	formVideoLoadBmp,
	NULL,
};

static HWND hwnd_videolayer = HWND_INVALID;
static BITMAP bkg;
static BmpLocation base_bmps[] = {
	{&bkg,"main/bg_1.png"},
	{NULL},
};

/* ---------------------------------------------------------------------------*/
/**
 * @brief formVideoLayerTimerProc1s 窗口相关定时函数
 *
 * @returns 1按键超时退出 0未超时退出
 */
/* ---------------------------------------------------------------------------*/
static void formVideoLayerTimerProc1s(HWND hwnd)
{
}


static void * loadBmpsThread(void *arg)
{
    int i;
    for (i=0; load_bmps_func[i] != NULL; i++) {
        load_bmps_func[i]();
    }
    return NULL;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief formVideoLayerProc 窗口回调函数
 *
 * @param hDlg
 * @param message
 * @param wParam
 * @param lParam
 *
 * @return
 */
/* ---------------------------------------------------------------------------*/
static int formVideoLayerProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case MSG_CREATE:
			{
				screenInit();
				Screen.Add(hWnd,"TFrmVL");
				hwnd_videolayer = hWnd;
                bmpsLoad(base_bmps);
				createThread(loadBmpsThread,"1");
                fontsLoad(font_load);
				SetTimer(hWnd, IDC_TIMER_1S, TIME_1S);
				// screensaverStart(LCD_ON);
			} break;

		case MSG_ERASEBKGND:
				drawBackground(hWnd,
						   (HDC)wParam,
						   (const RECT*)lParam,&bkg,0);
			 return 0;

		case MSG_MAIN_LOAD_BMP:
			{
                createThread(loadBmpsThread,NULL);
			} return 0;

		case MSG_TIMER:
			{
				if (wParam == IDC_TIMER_1S) {
                    formVideoLayerTimerProc1s(hWnd);
				}
			} return 0;

		case MSG_LBUTTONUP:
			{
                createFormMain(hWnd);
                // if (Public.LCDLight == 0) {
                    // screensaverStart(LCD_ON);
                    // return;
                // }
			} break;

		case MSG_DESTROY:
			{
				Screen.Del(hWnd);
				DestroyAllControls (hWnd);
			} return 0;

		case MSG_CLOSE:
			{

                KillTimer (hWnd,IDC_TIMER_1S);
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
 * @brief formVideoLayerCreate 创建主窗口
 *
 * @param AppProc
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
void formVideoLayerCreate(void)
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
    CreateInfo.spCaption = "cateye";
    CreateInfo.hMenu = 0;
    CreateInfo.hCursor = GetSystemCursor(IDC_ARROW);
    CreateInfo.hIcon = 0;
    CreateInfo.MainWindowProc = formVideoLayerProc;
    CreateInfo.lx = 0;
    CreateInfo.ty = 0;
    CreateInfo.rx = SCR_WIDTH;
    CreateInfo.by = SCR_HEIGHT;
    CreateInfo.iBkColor = GetWindowElementColor(WE_MAINC_THREED_BODY);
    CreateInfo.dwAddData = 0;
    CreateInfo.dwReserved = 0;
    CreateInfo.hHosting = HWND_DESKTOP;

	CreateMainWindow (&CreateInfo);
	if (hwnd_videolayer == HWND_INVALID)
		return ;
	ShowWindow(hwnd_videolayer, SW_SHOWNORMAL);

    MSG Msg;
	while (GetMessage(&Msg, hwnd_videolayer)) {
		TranslateMessage(&Msg);
		DispatchMessage(&Msg);
	}

    my_button->unregist();
    my_status->unregist();
    my_static->unregist();

    MainWindowThreadCleanup (hwnd_videolayer);
	hwnd_videolayer = 0;
}

