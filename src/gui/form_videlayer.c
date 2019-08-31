/* ---------------------------------------------------------------------------*
 *                      include head files
 *----------------------------------------------------------------------------*/

#include "externfunc.h"
#include "debug.h"
#include "screen.h"
#include "config.h"
#include "protocol.h"
#include "sensor_detector.h"
#include "thread_helper.h"

#include "my_button.h"
#include "my_status.h"
#include "my_static.h"
#include "my_title.h"
#include "my_battery.h"

#include "my_video.h"
#include "form_video.h"
/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
extern void createFormMain(HWND hMainWnd,void (*callback)(void));
extern int createFormPowerOff(HWND hMainWnd);
extern int createFormPowerOffLowPower(void);
extern void formMainLoadBmp(void);
extern void formSettingLoadBmp(void);
extern void formVideoLoadBmp(void);
extern void formMonitorLoadBmp(void);
extern void formSettingWifiLoadBmp(void);
extern void formPasswordLoadBmp(void);
extern void formSettingLocoalLoadBmp(void);
extern void formSettingStoreLoadBmp(void);
extern void formSettingQrcodeLoadBmp(void);
extern void formSettingUpdateLoadBmp(void);

extern void formVideoInitInterface(void);
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
typedef struct {
   void (*init)(void);
   MyControls ** controls;
}MyCtrls;

#define TIME_1S (100)

enum {
    IDC_TIMER_1S ,	// 1s定时器
    IDC_TIMER_NUM,
};
enum {
    UI_FORMAT_BGRA_8888 = 0x1000,
    UI_FORMAT_RGB_565,
};

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
PLOGFONT font36;
PLOGFONT font22;
PLOGFONT font20;

static MyCtrls ctrls[] = {
    {initMyButton,&my_button},
    {initMyStatus,&my_status},
    {initMyStatic,&my_static},
    {initMyTitle, &my_title},
    {initMyBattery,&my_battery},
    {NULL},
};
static FontLocation font_load[] = {
    {&font36,   36,FONT_WEIGHT_DEMIBOLD},
    {&font22,   22,FONT_WEIGHT_DEMIBOLD},
    {&font20,   20,FONT_WEIGHT_DEMIBOLD},
    {NULL}
};


static InitBmpFunc load_bmps_func[] = {
    formMainLoadBmp,
    formSettingLoadBmp,
	formMonitorLoadBmp,
    formSettingWifiLoadBmp,
	formPasswordLoadBmp,
	formSettingLocoalLoadBmp,
	formSettingStoreLoadBmp,
	formSettingQrcodeLoadBmp,
	formSettingUpdateLoadBmp,
	NULL,
};

static HWND hwnd_videolayer = HWND_INVALID;
static int flag_timer_stop = 0;
static int auto_close_lcd = AUTO_CLOSE_LCD;
static int sleep_timer = 0; // 进入睡眠倒计时
static int screen_on = 0;
static BmpLocation base_bmps[] = {
	{NULL},
};

/* ---------------------------------------------------------------------------*/
/**
 * @brief setAutoCloseLcdTime 设置关闭屏幕倒计时，同时重置睡眠倒计时
 *
 * @param count
 */
/* ---------------------------------------------------------------------------*/
static void setAutoCloseLcdTime(int count)
{
	auto_close_lcd = count;
	sleep_timer = 0;
}

void resetAutoSleepTimerLong(void)
{
	sleep_timer = SLEEP_LONG_TIMER;
}

void resetAutoSleepTimerShort(void)
{
	sleep_timer = SLEEP_TIMER;
}

static void enableAutoClose(void)
{
	flag_timer_stop = 0;	
	setAutoCloseLcdTime(AUTO_CLOSE_LCD);
	my_video->showLocalVideo();
}
void screenAutoCloseStop(void)
{
	setAutoCloseLcdTime(0);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief formVideoLayerScreenOn 点亮屏幕
 */
/* ---------------------------------------------------------------------------*/
void formVideoLayerScreenOn(void)
{
	screensaverStart(1);
	setAutoCloseLcdTime(AUTO_CLOSE_LCD);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief formVideoLayerGotoPoweroff 关机时调用
 */
/* ---------------------------------------------------------------------------*/
void formVideoLayerGotoPoweroff(void)
{
	my_video->hideVideo();
	screensaverStart(1);
	screenAutoCloseStop();
	createFormPowerOff(0);
}

static void interfaceLowPowerToPowerOff(void)
{
	my_video->hideVideo();
	screensaverStart(1);
	screenAutoCloseStop();
	createFormPowerOffLowPower();
	auto_close_lcd = 0;
	sleep_timer = SLEEP_TIMER;
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief formVideoLayerTimerProc1s 窗口相关定时函数
 *
 * @returns 1按键超时退出 0未超时退出
 */
/* ---------------------------------------------------------------------------*/
static void formVideoLayerTimerProc1s(HWND hwnd)
{
	if (auto_close_lcd) {
		// printf("auto_close_ld:%d\n",auto_close_lcd );
		if (--auto_close_lcd == 0) {
			screensaverStart(0);
			if(sleep_timer < SLEEP_TIMER)
				sleep_timer = SLEEP_TIMER;
		}
	}
	if (sleep_timer && auto_close_lcd == 0) {
		// printf("sleep:%d\n", sleep_timer);
		if (--sleep_timer == 0) {
			protocol_singlechip->cmdSleep();
		}
	}
}


static void * loadBmpsThread(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
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

				formVideoLoadBmp();
				HWND form = createFormPowerOff(hWnd);
				ShowWindow(form,SW_HIDE);
				form = createFormVideo(hWnd,FORM_VIDEO_TYPE_CAPTURE,NULL,0);
				ShowWindow(form,SW_HIDE);

				Screen.setCurrent("TFrmVL");
				my_video->showLocalVideo();
				formVideoInitInterface();
				screensaverStart(1);
				if (sensor) {
					sensor->interface->uiLowPowerToPowerOff = interfaceLowPowerToPowerOff;
				}
			} break;

		case MSG_ERASEBKGND:
				drawBackground(hWnd,
						   (HDC)wParam,
						   0,NULL,0);
			 return 0;

		case MSG_MAIN_LOAD_BMP:
			{
                createThread(loadBmpsThread,NULL);
			} return 0;

		case MSG_TIMER:
			{
				if (flag_timer_stop)
					return 0;
				if (wParam == IDC_TIMER_1S) {
                    formVideoLayerTimerProc1s(hWnd);
				}
			} return 0;

		case MSG_LBUTTONUP:
			{
				if (screen_on) {
					screen_on = 0;
					break;
				}
				flag_timer_stop = 1;
				setAutoCloseLcdTime(0);
                createFormMain(hWnd,enableAutoClose);
			} break;
		case MSG_LBUTTONDOWN:
			if (screensaverStart(1)) {
				screen_on = 1;
			}
			setAutoCloseLcdTime(AUTO_CLOSE_LCD);
			break;

		case MSG_ENABLE_WINDOW:
			enableAutoClose();
			break;
		case MSG_DISABLE_WINDOW:
			flag_timer_stop = 1;
			sleep_timer = 0;
			break;
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

    int i;
    for (i=0; ctrls[i].init != NULL; i++) {
        ctrls[i].init();
        (*ctrls[i].controls)->regist();
    }

    CreateInfo.dwStyle = WS_NONE;
	CreateInfo.dwExStyle = WS_EX_NONE;//WS_EX_AUTOSECONDARYDC;
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

    for (i=0; ctrls[i].init != NULL; i++) {
        (*ctrls[i].controls)->unregist();
    }

    MainWindowThreadCleanup (hwnd_videolayer);
	hwnd_videolayer = 0;
}

