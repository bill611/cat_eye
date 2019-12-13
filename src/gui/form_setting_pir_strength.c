/*
 * =============================================================================
 *
 *       Filename:  form_setting_pir_strength.c
 *
 *    Description:  PIR强度设置界面
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
#include "protocol.h"

#include "my_button.h"
#include "my_title.h"

#include "form_base.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static int formSettingPirStrengthProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
static void initPara(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);

static void buttonExitPress(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonLeftPress(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonRightPress(HWND hwnd, int id, int nc, DWORD add_data);

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
	IDC_TIMER_1S = IDC_FORM_SETTING_PIR_STRENGTH,
	IDC_BUTTON_EXIT,
	IDC_BUTTON_LEFT,
	IDC_BUTTON_RIGHT,
	IDC_STATIC_IMAGE,
	IDC_STATIC_TEXT,

	IDC_TITLE,
};


/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static BITMAP bmp_bkg_setting; // 背景

static int bmp_load_finished = 0;
static int flag_timer_stop = 0;

static BmpLocation bmp_load[] = {
	{&bmp_bkg_setting,BMP_LOCAL_PATH"grade_icon.png"},
    {NULL},
};

static MY_CTRLDATA ChildCtrls [] = {
    STATIC_IMAGE(432,220,160,160,IDC_STATIC_IMAGE,(DWORD)&bmp_bkg_setting),
    STATIC_LB(434,330,160,30,IDC_STATIC_TEXT,"1级",&font20,0xffffff),
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
	.name = "FsetPirStrength",
	.idc_timer = IDC_TIMER_1S,
	.dlgProc = formSettingPirStrengthProc,
	.dlgInitParam = &DlgInitParam,
	.initPara =  initPara,
	.auto_close_time_set = 30,
};

static MyCtrlButton ctrls_button[] = {
	{IDC_BUTTON_LEFT,	 MYBUTTON_TYPE_TWO_STATE|MYBUTTON_TYPE_TEXT_NULL,"left",225, 249,buttonLeftPress},
	{IDC_BUTTON_RIGHT,	 MYBUTTON_TYPE_TWO_STATE|MYBUTTON_TYPE_TEXT_NULL,"right",698,249,buttonRightPress},
	{0},
};
static MyCtrlTitle ctrls_title[] = {
	{
        IDC_TITLE, 
        MYTITLE_LEFT_EXIT,
        MYTITLE_RIGHT_NULL,
        0,0,1024,40,
        "人体检测灵敏度",
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
	flag_timer_stop = 0;	
}
static void reloadPirStrength(void)
{
	char buf[16] = {0};
	sprintf(buf,"%d级",g_config.pir_strength + 1);
	SendMessage(GetDlgItem(form_base->hDlg,IDC_STATIC_TEXT),MSG_SETTEXT,0,(LPARAM)buf);
	if (g_config.pir_strength == 0) {
		SendMessage(GetDlgItem(form_base->hDlg,IDC_BUTTON_LEFT),MSG_ENABLE,0,0);
	} else if (g_config.pir_strength == 2) {
		SendMessage(GetDlgItem(form_base->hDlg,IDC_BUTTON_RIGHT),MSG_ENABLE,0,0);
	} else {
		SendMessage(GetDlgItem(form_base->hDlg,IDC_BUTTON_LEFT),MSG_ENABLE,1,0);
		SendMessage(GetDlgItem(form_base->hDlg,IDC_BUTTON_RIGHT),MSG_ENABLE,1,0);
	}
}
/* ----------------------------------------------------------------*/
/**
 * @brief buttonLeftPress wifi设置
 *
 * @param hwnd
 * @param id
 * @param nc
 * @param add_data
 */
/* ----------------------------------------------------------------*/
static void buttonLeftPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
	if (g_config.pir_strength > 0) {
		g_config.pir_strength--;
		ConfigSavePublic();
		protocol_singlechip->cmdSetPirStrength(g_config.pir_strength);
	}
	reloadPirStrength();
}

static void buttonRightPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
	if (g_config.pir_strength < 2) {
		g_config.pir_strength++;
		ConfigSavePublic();
		protocol_singlechip->cmdSetPirStrength(g_config.pir_strength);
	}
	reloadPirStrength();
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

void formSettingPirStrengthLoadBmp(void)
{
    if (bmp_load_finished == 1)
        return;

	printf("[%s]\n", __FUNCTION__);
    bmpsLoad(bmp_load);
    my_button->bmpsLoad(ctrls_button,BMP_LOCAL_PATH);	
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
	reloadPirStrength();
}

/* ----------------------------------------------------------------*/
/**
 * @brief formSettingPirStrengthProc 窗口回调函数
 *
 * @param hDlg
 * @param message
 * @param wParam
 * @param lParam
 *
 * @return
 */
/* ----------------------------------------------------------------*/
static int formSettingPirStrengthProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    switch(message) // 自定义消息
    {
		case MSG_TIMER:
			{
				if (flag_timer_stop)
					return 0;
			} break;

		case MSG_ENABLE_WINDOW:
			enableAutoClose();
			break;
		case MSG_DISABLE_WINDOW:
			flag_timer_stop = 1;
			break;
        default:
            break;
    }
	if (form_base->baseProc(form_base,hDlg, message, wParam, lParam) == FORM_STOP)
		return 0;
    return DefaultDialogProc(hDlg, message, wParam, lParam);
}

int createFormSettingPirStrength(HWND hMainWnd,void (*callback)(void))
{
	HWND Form = Screen.Find(form_base_priv.name);
	if(Form) {
		Screen.setCurrent(form_base_priv.name);
		ShowWindow(Form,SW_SHOWNORMAL);
	} else {
        if (bmp_load_finished == 0) {
            // topMessage(hMainWnd,TOPBOX_ICON_LOADING,NULL );
            return 0;
        }
		form_base_priv.callBack = callback;
		form_base = formBaseCreate(&form_base_priv);
		return CreateMyWindowIndirectParam(form_base->priv->dlgInitParam,
				hMainWnd, form_base->priv->dlgProc, 0);
	}

	return 0;
}

