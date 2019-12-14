/*
 * =============================================================================
 *
 *       Filename:  form_setting_Time.c
 *
 *    Description:  时间设置界面
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
static int formSettingTimeProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
static void initPara(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);

static void buttonExitPress(HWND hwnd, int id, int nc, DWORD add_data);

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
	IDC_TIMER_1S = IDC_FORM_SETTING_TIME,
	IDC_BUTTON_EXIT,
	IDC_BUTTON_TITLE,
	IDC_MYSCROLL_HOUR,
	IDC_MYSCROLL_MIN,
	IDC_TITLE,
};


/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static int bmp_load_finished = 0;
static int flag_Time_stop = 0;
static int time_now = 0;
static void (*timeSaveCallback)(int );

static BmpLocation bmp_load[] = {
    {NULL},
};

static MY_CTRLDATA ChildCtrls [] = {
};

static MyCtrlButton ctrls_button[] = {
	{0},
};

static MyCtrlScroll ctrls_scroll[] = {
	{IDC_MYSCROLL_HOUR,	0,"时",0, 23,	287,100,180,400},
	{IDC_MYSCROLL_MIN,	0,"分",0, 59,	558,100,180,400},
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
	.name = "FsetTime",
	.idc_timer = IDC_TIMER_1S,
	.dlgProc = formSettingTimeProc,
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
        "时间设置",
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
	flag_Time_stop = 0;	
}

static void reloadTime(void)
{
    SendMessage(GetDlgItem(form_base->hDlg,IDC_MYSCROLL_HOUR),MSG_SET_NUM,time_now / 60,0);
    SendMessage(GetDlgItem(form_base->hDlg,IDC_MYSCROLL_MIN),MSG_SET_NUM,time_now % 60,0);
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
	struct tm *tm = getTime();
    int hour = SendMessage(GetDlgItem(form_base->hDlg,IDC_MYSCROLL_HOUR),MSG_GET_NUM,0,0);
    int min = SendMessage(GetDlgItem(form_base->hDlg,IDC_MYSCROLL_MIN),MSG_GET_NUM,0,0);
	if (timeSaveCallback) {
		time_now = hour * 60 + min;			
	} else {
		adjustdate(tm->tm_year+1900,tm->tm_mon+1,tm->tm_mday,hour,min,0);
	}
	ShowWindow(GetParent(hwnd),SW_HIDE);
}

void formSettingTimeLoadBmp(void)
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
}

/* ----------------------------------------------------------------*/
/**
 * @brief formSettingTimeProc 窗口回调函数
 *
 * @param hDlg
 * @param message
 * @param wParam
 * @param lParam
 *
 * @return
 */
/* ----------------------------------------------------------------*/
static int formSettingTimeProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    switch(message) // 自定义消息
    {
		case MSG_TIMER:
			{
				if (flag_Time_stop)
					return 0;
			} break;

		case MSG_FORM_SETTING_TIME_SET_TIME:
			time_now = wParam;
			reloadTime();
			break;
		case MSG_SHOWWINDOW:
			{
				if (wParam == SW_HIDE) {
					if (timeSaveCallback)				
						timeSaveCallback(time_now);
				}
			}break;
		case MSG_ENABLE_WINDOW:
			enableAutoClose();
			break;
		case MSG_DISABLE_WINDOW:
			flag_Time_stop = 1;
			break;
        default:
            break;
    }
	if (form_base->baseProc(form_base,hDlg, message, wParam, lParam) == FORM_STOP)
		return 0;
    return DefaultDialogProc(hDlg, message, wParam, lParam);
}

int createFormSettingTime(HWND hMainWnd,void (*callback)(void))
{
	HWND Form = Screen.Find(form_base_priv.name);
	if(Form) {
		Screen.setCurrent(form_base_priv.name);
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

int createFormSettingTimeReal(HWND hMainWnd,void (*callback)(void))
{
	struct tm *tm = getTime();
	timeSaveCallback = NULL;
	createFormSettingTime(hMainWnd,callback);
	SendMessage(GetDlgItem(form_base->hDlg,IDC_TITLE),
			MSG_MYTITLE_SET_TITLE,(WPARAM)"时间设置",0);
	SendMessage(form_base->hDlg, MSG_FORM_SETTING_TIME_SET_TIME,
			tm->tm_hour * 60 + tm->tm_min,0);
}
int createFormSettingTimeStart(HWND hMainWnd,void (*callback)(void),int time_buf,void (*saveCallback)(int ))
{
	timeSaveCallback = saveCallback;
	createFormSettingTime(hMainWnd,callback);
	SendMessage(GetDlgItem(form_base->hDlg,IDC_TITLE),
			MSG_MYTITLE_SET_TITLE,(WPARAM)"开始时间",0);
	SendMessage(form_base->hDlg, MSG_FORM_SETTING_TIME_SET_TIME, time_buf,0);
}
int createFormSettingTimeEnd(HWND hMainWnd,void (*callback)(void),int time_buf,void (*saveCallback)(int ))
{
	timeSaveCallback = saveCallback;
	createFormSettingTime(hMainWnd,callback);
	SendMessage(GetDlgItem(form_base->hDlg,IDC_TITLE),
			MSG_MYTITLE_SET_TITLE,(WPARAM)"结束时间",0);
	SendMessage(form_base->hDlg, MSG_FORM_SETTING_TIME_SET_TIME, time_buf,0);
}
