/*
 * =============================================================================
 *
 *       Filename:  form_setting_wifi.c
 *
 *    Description:  wifi设置界面
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

#include "form_base.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
int createFormPassword(HWND hMainWnd,void (*callback)(void),void (*getPassword)(char *));

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static int formSettingWifiProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
static void initPara(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);

static void buttonNotify(HWND hwnd, int id, int nc, DWORD add_data);

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
	IDC_TIMER_1S = IDC_FORM_SETTING_WIFI_STATR,
	IDC_STATIC_IMG_WARNING,
	IDC_STATIC_TEXT_WARNING,

	IDC_SCROLLVIEW,
	IDC_BUTTON_NETX,
	IDC_BUTTON_PREV,

	IDC_TITLE,
};

enum {
	SCROLLVIEW_ITEM_TYPE_TITLE,
	SCROLLVIEW_ITEM_TYPE_LIST,
};
struct ScrollviewItem {
	int type; // 选项类型
	int enable; // 是否选中
	int security; // 加密 0,1
	int strength; // 信号强度
	char text[32]; // 文字
};

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static HWND hScrollView;
static int flag_timer_stop = 0;
// static struct ScrollviewItem *wifi_list;
// TEST
static struct ScrollviewItem wifi_list[] = {
	{
		.type = SCROLLVIEW_ITEM_TYPE_LIST,
		.enable = 1,
		.security = 1,
		.strength = 1,
	},
	{
		.type = SCROLLVIEW_ITEM_TYPE_TITLE,
	},
	{
		.type = SCROLLVIEW_ITEM_TYPE_LIST,
		.security = 1,
		.strength = 0,
	},
	{
		.type = SCROLLVIEW_ITEM_TYPE_LIST,
		.security = 1,
		.strength = 1,
	},
	{
		.type = SCROLLVIEW_ITEM_TYPE_LIST,
		.security = 0,
		.strength = 2,
	},
};
static BITMAP bmp_warning; // 警告
static BITMAP bmp_security; // 加密
static BITMAP bmp_select; // 选中
static BITMAP bmp_enter; // 进入
static BITMAP bmp_wifi[3]; // wifi强度


static BmpLocation bmp_load[] = {
    {&bmp_warning,	BMP_LOCAL_PATH"ico_警告.png"},
    {&bmp_security,	BMP_LOCAL_PATH"ico_lock.png"},
    {&bmp_select,	BMP_LOCAL_PATH"ico_对.png"},
    {&bmp_enter,	BMP_LOCAL_PATH"ico_返回_1.png"},
    {&bmp_wifi[0],	BMP_LOCAL_PATH"ico_wifi_2.png"},
    {&bmp_wifi[1],	BMP_LOCAL_PATH"ico_wifi_1.png"},
    {&bmp_wifi[2],	BMP_LOCAL_PATH"ico_wifi.png"},
    {NULL},
};

static MY_CTRLDATA ChildCtrls [] = {
    STATIC_IMAGE(452,216,120,120,IDC_STATIC_IMG_WARNING,(DWORD)&bmp_warning),
    STATIC_LB(0,358,1024,25,IDC_STATIC_TEXT_WARNING,"WIFI已关闭",&font20,0xffffff),
    SCROLLVIEW(0,40,1024,580,IDC_SCROLLVIEW),
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
	.name = "Fsetwifi",
	.idc_timer = IDC_TIMER_1S,
	.dlgProc = formSettingWifiProc,
	.dlgInitParam = &DlgInitParam,
	.initPara =  initPara,
};

static MyCtrlButton ctrls_button[] = {
	{0},
};

static MyCtrlTitle ctrls_title[] = {
	{
        IDC_TITLE,
        MYTITLE_LEFT_EXIT,
        MYTITLE_RIGHT_SWICH,
        0,0,1024,40,
        "WIFI设置",
        "",
        0xffffff, 0x333333FF,
        buttonNotify,
    },
	{0},
};

static FormBase* form_base = NULL;

static void getPassword(char *password)
{
	printf("password:%s\n", password);
}

static void enableAutoClose(void)
{
	flag_timer_stop = 0;	
}

static void showWarning(HWND hwnd,int on_off)
{
    if (on_off) {
        ShowWindow(GetDlgItem(hwnd,IDC_STATIC_IMG_WARNING),SW_HIDE);
        ShowWindow(GetDlgItem(hwnd,IDC_STATIC_TEXT_WARNING),SW_HIDE);
        ShowWindow(GetDlgItem(hwnd,IDC_SCROLLVIEW),SW_SHOWNORMAL);
    } else {
        ShowWindow(GetDlgItem(hwnd,IDC_STATIC_IMG_WARNING),SW_SHOWNORMAL);
        ShowWindow(GetDlgItem(hwnd,IDC_STATIC_TEXT_WARNING),SW_SHOWNORMAL);
        ShowWindow(GetDlgItem(hwnd,IDC_SCROLLVIEW),SW_HIDE);
    }
}
/* ----------------------------------------------------------------*/
/**
 * @brief buttonNotify 退出按钮
 *
 * @param hwnd
 * @param id
 * @param nc
 * @param add_data
 */
/* ----------------------------------------------------------------*/
static void buttonNotify(HWND hwnd, int id, int nc, DWORD add_data)
{
    if (nc == MYTITLE_BUTTON_EXIT)
        ShowWindow(GetParent(hwnd),SW_HIDE);
    else if (nc == MYTITLE_BUTTON_SWICH) {
        g_config.net_config.enable = add_data;
        showWarning(GetParent(hwnd),add_data);
    }
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief scrollviewNotify 
 *
 * @param hwnd
 * @param id
 * @param nc
 * @param add_data
 */
/* ---------------------------------------------------------------------------*/
static void scrollviewNotify(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc == SVN_CLICKED) {
		int idx = SendMessage (hScrollView, SVM_GETCURSEL, 0, 0);
		struct ScrollviewItem *plist;
		plist = (struct ScrollviewItem *)SendMessage (hScrollView, SVM_GETITEMADDDATA, idx, 0);

		if (plist) {
			if (strcmp(plist->text,"附近网络")) {
				flag_timer_stop = 1;
				createFormPassword(hwnd,enableAutoClose,getPassword);
				printf("idx:%d,name:%s\n", idx,plist->text);
			}
		}
	}
}

void formSettingWifiLoadBmp(void)
{
    bmpsLoad(bmp_load);
}
static void myDrawItem (HWND hWnd, HSVITEM hsvi, HDC hdc, RECT *rcDraw)
{
#define FILL_BMP_STRUCT(left,top,img)  \
	FillBoxWithBitmap(hdc,left, top,img.bmWidth,img.bmHeight,&img)

#define DRAW_TABLE(rc,offset,color)  \
	do { \
		SetPenColor (hdc, color); \
		MoveTo (hdc, rc->left + offset, rc->top); \
		LineTo (hdc, rc->right,rc->top); \
		MoveTo (hdc, rc->left + offset, rc->bottom); \
		LineTo (hdc, rc->right,rc->bottom); \
	} while (0)

	struct ScrollviewItem *p_item = (struct ScrollviewItem *)scrollview_get_item_adddata (hsvi);
	SetBkMode (hdc, BM_TRANSPARENT);
	SetTextColor (hdc, PIXEL_lightwhite);
	SelectFont (hdc, font20);
	if(p_item->type == SCROLLVIEW_ITEM_TYPE_TITLE) {
		// 绘制表格
		DRAW_TABLE(rcDraw,0,0x12345678);
		TextOut (hdc, rcDraw->left + 30, rcDraw->top + 16, p_item->text);
	} else {
		if (p_item->enable) {
			FILL_BMP_STRUCT(rcDraw->left + 33,rcDraw->top + 18,bmp_select);
		}
		if (p_item->security) {
			FILL_BMP_STRUCT(rcDraw->left + 881,rcDraw->top + 15,bmp_security);
		}
		if (p_item->strength < 3)
			FILL_BMP_STRUCT(rcDraw->left + 919,rcDraw->top + 15,bmp_wifi[p_item->strength]);
		FILL_BMP_STRUCT(rcDraw->left + 968,rcDraw->top + 15,bmp_enter);
		// 绘制表格
		DRAW_TABLE(rcDraw,82,0x12345678);
		// 输出文字
		TextOut (hdc, rcDraw->left + 83, rcDraw->top + 15, p_item->text);
	}
}

static void loadWifiData(void)
{
	int i;
	SVITEMINFO svii;
	sprintf(wifi_list[0].text,"%s","123");
	sprintf(wifi_list[1].text,"%s","附近网络");
	sprintf(wifi_list[2].text,"%s","456");
	sprintf(wifi_list[3].text,"%s","789");
	struct ScrollviewItem *plist = wifi_list;
	for (i=0; i < 4; i++) {
		svii.nItemHeight = 60;
		svii.addData = (DWORD)plist;
		svii.nItem = i;
		SendMessage (hScrollView, SVM_ADDITEM, 0, (LPARAM)&svii);
		SendMessage (hScrollView, SVM_SETITEMADDDATA, i, (DWORD)plist);
		plist++;
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
    for (i=0; ctrls_title[i].idc != 0; i++) {
        ctrls_title[i].font = font20;
        createMyTitle(hDlg,&ctrls_title[i]);
    }
    for (i=0; ctrls_button[i].idc != 0; i++) {
        ctrls_button[i].font = font22;
        createMyButton(hDlg,&ctrls_button[i]);
    }
	hScrollView = GetDlgItem (hDlg, IDC_SCROLLVIEW);
	SendMessage (hScrollView, SVM_SETITEMDRAW, 0, (LPARAM)myDrawItem);
	//* 此处不能设置为回调函数，否则第一次返回为-1，minigui bug
	// SetNotificationCallback(hScrollView,scrollviewNotify);
	loadWifiData();
	SendMessage(GetDlgItem(hDlg,IDC_TITLE),
            MSG_MYTITLE_SET_SWICH, (WPARAM)g_config.net_config.enable, 0);
    showWarning(hDlg,g_config.net_config.enable);
}

/* ----------------------------------------------------------------*/
/**
 * @brief formSettingWifiProc 窗口回调函数
 *
 * @param hDlg
 * @param message
 * @param wParam
 * @param lParam
 *
 * @return
 */
/* ----------------------------------------------------------------*/
static int formSettingWifiProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    switch(message) // 自定义消息
    {
		case MSG_TIMER:
			{
				if (flag_timer_stop)
					return 0;
			} break;

		case MSG_COMMAND:
			{
				int id = LOWORD (wParam);
				int code = HIWORD (wParam);
				scrollviewNotify(hDlg,id,code,0);
				break;
			}
		default:
            break;
    }
	if (form_base->baseProc(form_base,hDlg, message, wParam, lParam) == FORM_STOP)
		return 0;
    return DefaultDialogProc(hDlg, message, wParam, lParam);
}

int createFormSettingWifi(HWND hMainWnd,void (*callback)(void))
{
	HWND Form = Screen.Find(form_base_priv.name);
	if(Form) {
		ShowWindow(Form,SW_SHOWNORMAL);
	} else {
		form_base_priv.hwnd = hMainWnd;
		form_base_priv.callBack = callback;
		form_base = formBaseCreate(&form_base_priv);
		return CreateMyWindowIndirectParam(form_base->priv->dlgInitParam,
				form_base->priv->hwnd,
				form_base->priv->dlgProc, 0);
	}

	return 0;
}

