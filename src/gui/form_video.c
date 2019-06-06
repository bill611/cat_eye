/*
 * =============================================================================
 *
 *       Filename:  FormVideo.c
 *
 *    Description:  视频通话窗口，包括录像，抓拍等
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

#include "debug.h"
#include "protocol.h"
/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
extern void formSettingLoadBmp(void);

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static int formVideoProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam);
static void initPara(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
static void formVideoTimerProc1s(HWND hwnd);

static void buttonHangupPress(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonUnlockPress(HWND hwnd, int id, int nc, DWORD add_data);

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#if DBG_FORM_MAIN > 0
	#define DBG_P( x... ) printf( x )
#else
	#define DBG_P( x... )
#endif


#define BMP_LOCAL_PATH "video/"

#define TIME_1S (10 * 5)
#define TIME_100MS (TIME_1S / 10)

enum {
    IDC_TIMER_1S = IDC_FORM_VIDEO_START,

    IDC_MYSTATIC_TIME,
    IDC_MYSTATIC_BATTERY,

    IDC_BUTTON_UNLOCK,
    IDC_BUTTON_HANGUP,

    IDC_STATE_COM,

};

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static BmpLocation base_bmps[] = {
	{NULL},
};

static MyCtrlStatus ctrls_status[] = {
	{0},
};
static MyCtrlStatic ctrls_static[] = {
    {IDC_MYSTATIC_TIME,   MYSTATIC_TYPE_TEXT,0,0,1024,40,"",0xffffff,0x00000060},
    {IDC_MYSTATIC_BATTERY,MYSTATIC_TYPE_TEXT,910,0,45,34,"",0xffffff,0x00000000},
	{0},
};

static MyCtrlButton ctrls_button[] = {
	{IDC_BUTTON_HANGUP,MYBUTTON_TYPE_TWO_STATE,"挂断",80,451,buttonHangupPress},
	{IDC_BUTTON_UNLOCK,MYBUTTON_TYPE_TWO_STATE,"开门",338,451,buttonUnlockPress},
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

static FormBasePriv form_base_priv = {
	.name = "Fvideo",
	.idc_timer = IDC_TIMER_1S,
	.dlgProc = formVideoProc,
	.dlgInitParam = &DlgInitParam,
	.initPara =  initPara,
};

static int bmp_load_finished = 0;
static int flag_timer_stop = 0;
static int form_type = FORM_VIDEO_TYPE_CAPTURE;
static FormBase* form_base = NULL;

static void enableAutoClose(void)
{
	flag_timer_stop = 0;
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief formVideoTimerProc1s 窗口相关定时函数
 *
 * @returns 1按键超时退出 0未超时退出
 */
/* ---------------------------------------------------------------------------*/
static void formVideoTimerProc1s(HWND hwnd)
{
	// 更新时间

}

static void buttonHangupPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
	protocol_talk->hangup(NULL);
	ShowWindow(GetParent(hwnd),SW_HIDE);
}
static void buttonUnlockPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief formVideoLoadBmp 加载主界面图片
 */
/* ---------------------------------------------------------------------------*/
void formVideoLoadBmp(void)
{
    if (bmp_load_finished == 1)
        return;
	printf("[%s]\n", __FUNCTION__);
    bmpsLoad(base_bmps);
    my_button->bmpsLoad(ctrls_button,BMP_LOCAL_PATH);
    my_status->bmpsLoad(ctrls_status,BMP_LOCAL_PATH);
    bmp_load_finished = 1;
}
static void formVideoReleaseBmp(void)
{
	printf("[%s]\n", __FUNCTION__);
	bmpsRelease(base_bmps);
}

static void updateDisplay(HWND hDlg)
{
	if (form_type == FORM_VIDEO_TYPE_CAPTURE) {
		ShowWindow(GetDlgItem(hDlg,IDC_BUTTON_UNLOCK),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_BUTTON_HANGUP),SW_HIDE);
	} else if (form_type == FORM_VIDEO_TYPE_RECORD) {
		ShowWindow(GetDlgItem(hDlg,IDC_BUTTON_UNLOCK),SW_HIDE);
		ShowWindow(GetDlgItem(hDlg,IDC_BUTTON_HANGUP),SW_SHOWNORMAL);
		myMoveWindow(GetDlgItem(hDlg,IDC_BUTTON_HANGUP), 467,451);
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
 * @brief formVideoProc 窗口回调函数
 *
 * @param hDlg
 * @param message
 * @param wParam
 * @param lParam
 *
 * @return
 */
/* ---------------------------------------------------------------------------*/
static int formVideoProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case MSG_INITDIALOG:
			{
				// SetTimer(hDlg, IDC_TIMER_1S, TIME_1S);
                // HDC hdc = GetClientDC (hDlg);
                // mlsSetSlaveScreenInfo(hdc,MLS_INFOMASK_ALL,1,0,
                        // MLS_BLENDMODE_COLORKEY,0x00000100,0x00,3);
                // mlsEnableSlaveScreen(hdc,1);
			} break;

		// case MSG_ERASEBKGND:
			// {
				// drawBackground(hDlg,
						   // (HDC)wParam,
						   // (const RECT*)lParam,NULL,0);
			// } return 0;


		case MSG_TIMER:
			{
				if (flag_timer_stop)
					return 0;
				if (wParam == IDC_TIMER_1S){
					formVideoTimerProc1s(hDlg);
				}
			} break;

		case MSG_SHOWWINDOW:
			{
				if (wParam == SW_SHOWNORMAL)
					updateDisplay(hDlg);
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



int createFormVideo(HWND hMainWnd,int type,void (*callback)(void))
{
	HWND Form = Screen.Find(form_base_priv.name);
	form_type = type;
	if(Form) {
		DPRINT("video show\n");
		ShowWindow(Form,SW_SHOWNORMAL);
	} else {
		DPRINT("video create,main:%d\n",hMainWnd);
        if (bmp_load_finished == 0) {
            // topMessage(hVideoWnd,TOPBOX_ICON_LOADING,NULL );
            return 0;
        }
		form_base_priv.callBack = callback;
		form_base = formBaseCreate(&form_base_priv);
		return CreateMyWindowIndirectParam(form_base->priv->dlgInitParam,
				hMainWnd, form_base->priv->dlgProc, 0);
	}

	return 0;
}

void interfaceCreateFormVideoDirect(void *arg)
{
	printf("[%s]\n", __func__);
	int type = *(int *)arg;
	createFormVideo(0,type,NULL); 
}
void interfaceHangup(void *arg)
{
	ShowWindow(form_base->hDlg,SW_HIDE);
}

void formVideoInitInterface(void)
{
	protocol_talk->cblIncomingCall(interfaceCreateFormVideoDirect);	
}
