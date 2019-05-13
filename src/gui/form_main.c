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
#include <time.h>
#include "externfunc.h"
#include "debug.h"
#include "screen.h"
#include "config.h"
#include "thread_helper.h"
#include "hal_battery.h"

#include "my_button.h"
#include "my_status.h"
#include "my_static.h"

#include "form_video.h"
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
	IDC_TIMER_1S  = IDC_FORM_MAIN_START,
    IDC_MYSTATUS_WIFI ,
    IDC_MYSTATUS_SDCARD,
    IDC_MYSTATUS_BATTERY,

    IDC_MYSTATIC_DATE,
    IDC_MYSTATIC_BATTERY,

    IDC_BUTTON_RECORD,
    IDC_BUTTON_CAPTURE,
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

static BmpLocation base_bmps[] = {
	{NULL},
};

static MyCtrlStatus ctrls_status[] = {
    {IDC_MYSTATUS_WIFI,   "wifi",16,10,5},
	{IDC_MYSTATUS_SDCARD, "sdcard",54,8,1},
	{IDC_MYSTATUS_BATTERY,"Battery",963,10,3},
	{0},
};
static MyCtrlStatic ctrls_static[] = {
    {IDC_MYSTATIC_DATE,   MYSTATIC_TYPE_TEXT,0,0,1024,40,"",0xffffff,0x00000060},
    {IDC_MYSTATIC_BATTERY,MYSTATIC_TYPE_TEXT,910,0,45,34,"",0xffffff,0x00000000},
	{0},
};

static MyCtrlButton ctrls_button[] = {
	{IDC_BUTTON_RECORD,	MYBUTTON_TYPE_TWO_STATE,"记录",80,451,buttonRecordPress},
	{IDC_BUTTON_CAPTURE,MYBUTTON_TYPE_TWO_STATE,"抓拍",338,451,buttonCapturePress},
	{IDC_BUTTON_VIDEO,	MYBUTTON_TYPE_TWO_STATE,"录像",597,451,buttonVideoPress},
	{IDC_BUTTON_SETTING,MYBUTTON_TYPE_TWO_STATE,"设置",855,451,buttonSettingPress},
	{0},
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
	.idc_timer = IDC_TIMER_1S,
	.dlgProc = formMainProc,
	.dlgInitParam = &DlgInitParam,
	.initPara =  initPara,
};

static int bmp_load_finished = 0;
static int flag_timer_stop = 0;
static FormBase* form_base = NULL;
static HWND hwnd_main = HWND_INVALID;
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

static void enableAutoClose(void)
{
	flag_timer_stop = 0;	
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
	// 更新网络状态
	static int net_level_old = 0;
	int net_level = net_detect();
	if (net_level != net_level_old) {
		net_level_old = net_level;
		SendMessage(GetDlgItem (hwnd, IDC_MYSTATUS_WIFI),
				MSG_MYSTATUS_SET_LEVEL,net_level,0);
	}
	// 更新电量
	static int power_old = 0;
    char power_lever[16] = {0};
	int power = halBatteryGetEle();
	if (power_old == 0 || power_old != power) {
		power_old = power;
		if (power >= 80) {
			SendMessage(GetDlgItem (hwnd, IDC_MYSTATUS_BATTERY),
					MSG_MYSTATUS_SET_LEVEL,1,0);
		}
		sprintf(power_lever,"%d%%",power);
		SendMessage(GetDlgItem (hwnd, IDC_MYSTATIC_BATTERY),
				MSG_MYSTATIC_SET_TITLE,(WPARAM)power_lever,0);
	}
	// 更新时间 
	static struct tm *tm_old = NULL;
    char buf[16] = {0};
	struct tm *tm = getTime();
	if ((tm_old == NULL) || (tm_old->tm_hour != tm->tm_hour) || (tm_old->tm_min != tm->tm_min)) {
		tm_old = tm; 
		if (tm->tm_hour > 12) {
			sprintf(buf,"%d:%02d PM",tm->tm_hour-12,tm->tm_min);
		} else {
			sprintf(buf,"%d:%02d AM",tm->tm_hour,tm->tm_min);
		}
		SendMessage(GetDlgItem (hwnd, IDC_MYSTATIC_DATE),
				MSG_MYSTATIC_SET_TITLE,(WPARAM)buf,0);
	}

}

static void buttonRecordPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
	flag_timer_stop = 1;
}
static void buttonCapturePress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
	flag_timer_stop = 1;
	createFormVideo(GetParent(hwnd),FORM_VIDEO_TYPE_CAPTURE,enableAutoClose);
}
static void buttonVideoPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
	flag_timer_stop = 1;
	createFormVideo(GetParent(hwnd),FORM_VIDEO_TYPE_RECORD,enableAutoClose);
}
static void buttonSettingPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
	flag_timer_stop = 1;
    createFormSetting(GetParent(hwnd),enableAutoClose);
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
				if (flag_timer_stop)
					return 0;
				if (wParam == IDC_TIMER_1S) {
					formMainTimerProc1s(hDlg);
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
