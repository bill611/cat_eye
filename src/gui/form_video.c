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
#include <string.h>
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

#include "my_video.h"
#include "debug.h"
#include "protocol.h"
/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
extern void formSettingLoadBmp(void);
extern void screenAutoCloseStop(void);

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static int formVideoProc(HWND hWnd, int message, WPARAM wParam, LPARAM lParam);
static void initPara(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
static void formVideoTimerProc1s(HWND hwnd);

static void buttonHangupPress(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonUnlockPress(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonAnswerPress(HWND hwnd, int id, int nc, DWORD add_data);

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
    IDC_MYSTATIC_TITLE,
    IDC_MYSTATIC_TITLE_IMG,

    IDC_BUTTON_UNLOCK,
    IDC_BUTTON_HANGUP,
    IDC_BUTTON_CAPTURE,
    IDC_BUTTON_RECORD,
    IDC_BUTTON_ANSWER,

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
    {IDC_MYSTATIC_TITLE_IMG,MYSTATIC_TYPE_IMG_ONLY,0,0,1024,40,"",0xffffff,0x00000060},
    {IDC_MYSTATIC_TIME,   MYSTATIC_TYPE_TEXT_ONLY,0,0,200,40,"",0xffffff,0x00000000},
    {IDC_MYSTATIC_BATTERY,MYSTATIC_TYPE_TEXT,910,0,45,34,"",0xffffff,0x00000000},
    {IDC_MYSTATIC_TITLE,  MYSTATIC_TYPE_TEXT_ONLY,312,0,400,40,"",0xffffff,0x00000000},
	{0},
};

static MyCtrlButton ctrls_button[] = {
	{IDC_BUTTON_HANGUP,MYBUTTON_TYPE_TWO_STATE,"挂断",80,451,buttonHangupPress},
	{IDC_BUTTON_UNLOCK,MYBUTTON_TYPE_TWO_STATE,"开门",338,451,buttonUnlockPress},
	{IDC_BUTTON_CAPTURE,MYBUTTON_TYPE_TWO_STATE,"抓拍",338,451,buttonUnlockPress},
	{IDC_BUTTON_RECORD,MYBUTTON_TYPE_TWO_STATE,"录像",338,451,buttonUnlockPress},
	{IDC_BUTTON_ANSWER,MYBUTTON_TYPE_TWO_STATE,"接听",338,451,buttonAnswerPress},
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
static int form_type = FORM_VIDEO_TYPE_CAPTURE;
static FormBase* form_base = NULL;
static int auto_close_time = 0;
static int window_show_status = 0;

/* ---------------------------------------------------------------------------*/
/**
 * @brief formVideoTimerProc1s 窗口相关定时函数
 *
 * @returns 1按键超时退出 0未超时退出
 */
/* ---------------------------------------------------------------------------*/
static void formVideoTimerProc1s(HWND hwnd)
{
	static int call_time_old = 0;
	// 更新时间
	int call_time = my_video->videoGetCallTime();
	if (call_time != call_time_old) {
		char buf[32] = {0};
		sprintf(buf,"剩余时间: %d s",call_time);
		SendMessage(GetDlgItem (form_base->hDlg, IDC_MYSTATIC_TIME),
				MSG_MYSTATIC_SET_TITLE,(WPARAM)buf,0);
	}
	call_time_old = call_time;
	if (auto_close_time) {
		if (--auto_close_time == 0) {
			ShowWindow(form_base->hDlg,SW_HIDE);
		}
	}
}

static void buttonHangupPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
	if (form_type == FORM_VIDEO_TYPE_RECORD) {
		my_video->recordStop();
        ShowWindow(form_base->hDlg,SW_HIDE);
	} else {
		my_video->videoHangup();
	}
}
static void buttonUnlockPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
	if (protocol_talk)
		protocol_talk->unlock();
}

static void buttonAnswerPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
	if (my_video)
		my_video->videoAnswer(0,DEV_TYPE_ENTRANCEMACHINE);
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

static void updateTitle(char *name)
{
	if (name) {
		SendMessage(GetDlgItem (form_base->hDlg, IDC_MYSTATIC_TITLE),
				MSG_MYSTATIC_SET_TITLE,(WPARAM)name,0);
	}
}

static void updateDisplay(HWND hDlg)
{
	int button_status[] = {0,0,0,0,0};
	int button_num = IDC_BUTTON_UNLOCK;
	switch(form_type) 
	{
		case FORM_VIDEO_TYPE_CAPTURE:
			updateTitle("正在抓拍");
			break;
		case FORM_VIDEO_TYPE_MONITOR:
			button_status[IDC_BUTTON_HANGUP - button_num] = 1;
			myMoveWindow(GetDlgItem(hDlg,IDC_BUTTON_HANGUP), 467,451);
			break;
		case FORM_VIDEO_TYPE_RECORD:
			updateTitle("正在录像");
			button_status[IDC_BUTTON_HANGUP - button_num] = 1;
			myMoveWindow(GetDlgItem(hDlg,IDC_BUTTON_HANGUP), 467,451);
			break;
		case FORM_VIDEO_TYPE_TALK :
			button_status[IDC_BUTTON_UNLOCK - button_num] = 1;
			// button_status[IDC_BUTTON_ANSWER - button_num] = 1;
			button_status[IDC_BUTTON_HANGUP - button_num] = 1;
			myMoveWindow(GetDlgItem(hDlg,IDC_BUTTON_UNLOCK), 208,451);
			// myMoveWindow(GetDlgItem(hDlg,IDC_BUTTON_ANSWER), 467,451);
			myMoveWindow(GetDlgItem(hDlg,IDC_BUTTON_HANGUP), 726,451);
			break;
		case FORM_VIDEO_TYPE_OUTDOOR:
			if (protocol_talk->type == PROTOCOL_TALK_3000) {
				button_status[IDC_BUTTON_UNLOCK - button_num] = 1;
				button_status[IDC_BUTTON_ANSWER - button_num] = 1;
				button_status[IDC_BUTTON_HANGUP - button_num] = 1;
				myMoveWindow(GetDlgItem(hDlg,IDC_BUTTON_UNLOCK), 208,451);
				myMoveWindow(GetDlgItem(hDlg,IDC_BUTTON_ANSWER), 467,451);
				myMoveWindow(GetDlgItem(hDlg,IDC_BUTTON_HANGUP), 726,451);
			} else {
				button_status[IDC_BUTTON_CAPTURE - button_num] = 1;
				button_status[IDC_BUTTON_RECORD - button_num] = 1;
				button_status[IDC_BUTTON_HANGUP - button_num] = 1;
				myMoveWindow(GetDlgItem(hDlg,IDC_BUTTON_CAPTURE), 208,451);
				myMoveWindow(GetDlgItem(hDlg,IDC_BUTTON_HANGUP), 467,451);
				myMoveWindow(GetDlgItem(hDlg,IDC_BUTTON_RECORD), 726,451);
			}
			break;
		default:
			break;
	}
	int i;
	for (i=0; i<sizeof(button_status)/sizeof(button_status[0]); i++) {
		if (button_status[i] == 0)	
			ShowWindow(GetDlgItem(hDlg,button_num + i),SW_HIDE);
		else
			ShowWindow(GetDlgItem(hDlg,button_num + i),SW_SHOWNORMAL);
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
		case MSG_TIMER:
			{
				// video界面不自动关闭
				if (wParam == IDC_TIMER_1S){
					formVideoTimerProc1s(hDlg);
					return 0;
				}
			} break;

		case MSG_SHOWWINDOW:
			{
				if (wParam == SW_SHOWNORMAL) {
					if (window_show_status == 1)
						return 0;
					window_show_status = 1;
					updateDisplay(hDlg);
					SendMessage(Screen.getCurrent(),MSG_DISABLE_WINDOW,0,0);	
				} else if (wParam == SW_HIDE) {
					if (window_show_status == 0)
						return 0;
					window_show_status = 0;
					SendMessage(Screen.getCurrent(),MSG_ENABLE_WINDOW,0,0);	
				}
			} break;
		case MSG_LBUTTONDOWN:
			{
                // if (Public.LCDLight == 0) {
                    // screensaverSet(LCD_ON);
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



int createFormVideo(HWND hMainWnd,int type,void (*callback)(void),int count)
{
	HWND Form = Screen.Find(form_base_priv.name);
	form_type = type;
	auto_close_time = count;
	if(Form) {
		screenAutoCloseStop();
		ShowWindow(Form,SW_SHOWNORMAL);
	} else {
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

static void interfaceCreateFormVideoDirect(int type,char *name)
{
	switch (type) 
	{
		case DEV_TYPE_UNDEFINED:
		case DEV_TYPE_HOUSEHOLDAPP:
		case DEV_TYPE_SECURITYSTAFFAPP:
		case DEV_TYPE_INNERDOORMACHINE:
			createFormVideo(0,FORM_VIDEO_TYPE_MONITOR,NULL,0); 
			break;
		case DEV_TYPE_ENTRANCEMACHINE:
		case DEV_TYPE_HOUSEENTRANCEMACHINE:
			screensaverSet(1);
			if (protocol_talk->type == PROTOCOL_TALK_3000)
				createFormVideo(0,FORM_VIDEO_TYPE_OUTDOOR,NULL,0); 
			else
				createFormVideo(0,FORM_VIDEO_TYPE_TALK,NULL,0); 
			break;
		default:
			break;
	}
	updateTitle(name);
}
static void interfaceHangup(void)
{
	ShowWindow(form_base->hDlg,SW_HIDE);
}
static void interfaceAnswer(char *name)
{
	updateTitle(name);
	ShowWindow(GetDlgItem(form_base->hDlg,IDC_BUTTON_ANSWER),SW_HIDE);
}

void formVideoInitInterface(void)
{
	if (protocol_talk) {
		protocol_talk->uiShowFormVideo = interfaceCreateFormVideoDirect;
		protocol_talk->uiHangup = interfaceHangup;
		protocol_talk->uiAnswer = interfaceAnswer;
	}
}
