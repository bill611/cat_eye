/*
 * =============================================================================
 *
 *       Filename:  form_topmessage.c
 *
 *    Description:  Qrcode设置界面
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
#include <string.h>
#include "externfunc.h"
#include "screen.h"

#include "my_button.h"
#include "my_title.h"
#include "config.h"
#include "protocol.h"

#include "form_base.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static void buttonCancel(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonConfirm(HWND hwnd, int id, int nc, DWORD add_data);
static int formTopmessageProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
static void initPara(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);

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
	IDC_TIMER_1S = IDC_FORM_TOPMESSAGE,
	IDC_STATIC_TEXT_TITLE,
	IDC_STATIC_TEXT_CONTENT,

	IDC_STATIC_IMAGE_IMEI,
	IDC_STATIC_IMAGE_APP_URL,

	IDC_BUTTON_COMFIRM,
	IDC_BUTTON_CANCEL,
};

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static BITMAP bmp_bkg;
static char txt_title[128],txt_content[521];
static void (*callbackConfirm)(void);
static void (*callbackCancel)(void);

static BmpLocation bmp_load[] = {
	{&bmp_bkg,BMP_LOCAL_PATH"bg_fact_reset.png"},
	// {&bmp_null,BMP_LOCAL_PATH"cancel_btn.png"},
	// {&bmp_null,BMP_LOCAL_PATH"confirm_btn.png"},
    {NULL},
};

static MY_CTRLDATA ChildCtrls [] = {
};

static MY_DLGTEMPLATE DlgInitParam =
{
    WS_NONE,
    WS_EX_NONE,
    282,160,460,280,
    "",
    0, 0,       //menu and icon is null
    sizeof(ChildCtrls)/sizeof(MY_CTRLDATA),
    ChildCtrls, //pointer to control array
    0           //additional data,must be zero
};

static FormBasePriv form_base_priv= {
	.name = "FsetTopmessage",
	.idc_timer = IDC_TIMER_1S,
	.dlgProc = formTopmessageProc,
	.dlgInitParam = &DlgInitParam,
	.initPara =  initPara,
	.bmp_bkg = &bmp_bkg,
};

static MyCtrlButton ctrls_button[] = {
	{IDC_BUTTON_CANCEL,MYBUTTON_TYPE_TWO_STATE|MYBUTTON_TYPE_TEXT_CENTER,"取消",0,230,buttonCancel},
	{IDC_BUTTON_COMFIRM,MYBUTTON_TYPE_TWO_STATE|MYBUTTON_TYPE_TEXT_CENTER,"确认",231,230,buttonConfirm},
	{0},
};

static FormBase* form_base = NULL;

static void buttonCancel(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
	if (callbackCancel)
		callbackCancel();
	ShowWindow(GetParent(hwnd),SW_HIDE);
}

static void buttonConfirm(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
	if (callbackConfirm)
		callbackConfirm();
	ShowWindow(GetParent(hwnd),SW_HIDE);
}

static void paint(HWND hWnd,HDC hdc)
{
	RECT rc_text;
	SetTextColor(hdc,RGBA2Pixel (hdc, 0x38, 0x38, 0x38, 0xFF));
	SelectFont (hdc, font20);
	rc_text.left = 0;
	rc_text.right = 460;
	rc_text.top = 0;
	rc_text.bottom = 63;
	DrawText (hdc,txt_title, -1, &rc_text,
			DT_CENTER | DT_VCENTER | DT_WORDBREAK  | DT_SINGLELINE);
	rc_text.left = 0;
	rc_text.right = 460;
	rc_text.top = 63;
	rc_text.bottom = 230;
	DrawText (hdc,txt_content, -1, &rc_text,
			DT_CENTER | DT_VCENTER | DT_WORDBREAK  | DT_SINGLELINE);
}

static void formTopmessageLoadBmp(void)
{
	printf("[%s]\n", __FUNCTION__);
    bmpsLoad(bmp_load);
    my_button->bmpsLoad(ctrls_button,BMP_LOCAL_PATH);
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
	for (i=0; ctrls_button[i].idc != 0; i++) {
        ctrls_button[i].font = font22;
		if (ctrls_button[i].idc == IDC_BUTTON_COMFIRM) {
			ctrls_button[i].color_nor = 0x10B7F5;
		}
        createMyButton(hDlg,&ctrls_button[i]);
	}
}

/* ----------------------------------------------------------------*/
/**
 * @brief formTopmessageProc 窗口回调函数
 *
 * @param hDlg
 * @param message
 * @param wParam
 * @param lParam
 *
 * @return
 */
/* ----------------------------------------------------------------*/
static int formTopmessageProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    HDC         hdc;
    switch(message) // 自定义消息
    {
		case MSG_PAINT:
			hdc = BeginPaint (hDlg);
			paint(hDlg,hdc);
			EndPaint (hDlg, hdc);
			return 0;
		default:
            break;
    }
	if (form_base->baseProc(form_base,hDlg, message, wParam, lParam) == FORM_STOP)
		return 0;
    return DefaultDialogProc(hDlg, message, wParam, lParam);
}


int createFormTopmessage(HWND hMainWnd,char *title,char *content,void (*fConfirm)(void),void (*fCancel)(void))
{
	HWND Form = Screen.Find(form_base_priv.name);
	if (title)
		strcpy(txt_title,title);
	if (content)
		strcpy(txt_content,content);
	callbackConfirm = fConfirm;
	callbackCancel = fCancel;
	if(Form) {
		ShowWindow(Form,SW_SHOWNORMAL);
	} else {
		formTopmessageLoadBmp();
		form_base = formBaseCreate(&form_base_priv);
		return CreateMyWindowIndirectParam(form_base->priv->dlgInitParam,
				hMainWnd, form_base->priv->dlgProc, 0);
	}

	return 0;
}

void topMsgDoorbell(void)
{
	createFormTopmessage(0,"提示","有人在按门铃",NULL,NULL);
}

void topMsgCammerError(void)
{
	createFormTopmessage(0,"提示","摄像头异常",NULL,NULL);
}
