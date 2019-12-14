/*
 * =============================================================================
 *
 *       Filename:  form_setting_Mute.c
 *
 *    Description:  免扰设置界面
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
#include "screen.h"

#include "iwlib.h"
#include "my_button.h"
#include "my_title.h"
#include "config.h"
#include "externfunc.h"
#include "thread_helper.h"

#include "form_base.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
extern int createFormSettingTimeStart(HWND hMainWnd,void (*callback)(void),int time_buf,void (*saveCallback)(int ));
extern int createFormSettingTimeEnd(HWND hMainWnd,void (*callback)(void),int time_buf,void (*saveCallback)(int ));

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static int formSettingMuteProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
static void initPara(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
static void MuteLoadData(void *ap_info,int ap_cnt);

static void buttonTitleNotify(HWND hwnd, int id, int nc, DWORD add_data);
static void reloadTimer(HWND hwnd);
static void saveStartTime(int time_buf);
static void saveEndTime(int time_buf);

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
	IDC_TIMER_1S = IDC_FORM_SETTING_MUTE,
	IDC_STATIC_IMG_WARNING,
	IDC_STATIC_TEXT_WARNING,

	IDC_SCROLLVIEW,

	IDC_TITLE,
};

enum {
	SCROLLVIEW_ITEM_TYPE_TITLE,
	SCROLLVIEW_ITEM_TYPE_LIST,
};

struct ScrollviewItem {
	char title[32]; // 左边标题
	char text[32];  // 右边文字
	int (*callback)(HWND,void (*)(void),int,void (*)(int) ); // 点击回调函数
	void (*saveCallback)(int ); // 设置返回保存回调函数
	int index;  // 元素位置
	int time_buf;  // 当前设定时间
};

// 控件滑动相关操作
#define FILL_BMP_STRUCT(left,top,img)  \
	FillBoxWithBitmap(hdc,left, top,img.bmWidth,img.bmHeight,&img)

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static HWND hScrollView;
static int bmp_load_finished = 0;
static int flag_timer_stop = 0;

static BITMAP bmp_warning; // 警告
static BITMAP bmp_security; // 加密
static BITMAP bmp_select; // 选中
static BITMAP bmp_enter; // 进入

static struct ScrollviewItem locoal_list[] = {
	{"开始时间","",createFormSettingTimeStart,saveStartTime},
	{"结束时间","",createFormSettingTimeEnd,saveEndTime},
	{0},
};

static BmpLocation bmp_load[] = {
    {&bmp_warning,	BMP_LOCAL_PATH"ico_警告.png"},
    {&bmp_security,	BMP_LOCAL_PATH"ico_lock.png"},
    {&bmp_select,	BMP_LOCAL_PATH"ico_对.png"},
    {&bmp_enter,	BMP_LOCAL_PATH"ico_返回_1.png"},
    {NULL},
};

static MY_CTRLDATA ChildCtrls [] = {
    STATIC_IMAGE(452,216,120,120,IDC_STATIC_IMG_WARNING,(DWORD)&bmp_warning),
    STATIC_LB(0,358,1024,25,IDC_STATIC_TEXT_WARNING,"免扰已关闭，请点击开关开启",&font20,0xffffff),
    SCROLLVIEW(0,40,1024,390,IDC_SCROLLVIEW),
};

static MY_DLGTEMPLATE DlgInitParam =
{
    WS_NONE,
	WS_EX_NONE ,//| WS_EX_AUTOSECONDARYDC,
    0,0,SCR_WIDTH,SCR_HEIGHT,
    "",
    0, 0,       //menu and icon is null
    sizeof(ChildCtrls)/sizeof(MY_CTRLDATA),
    ChildCtrls, //pointer to control array
    0           //additional data,must be zero
};

static FormBasePriv form_base_priv= {
	.name = "FsetMute",
	.idc_timer = IDC_TIMER_1S,
	.dlgProc = formSettingMuteProc,
	.dlgInitParam = &DlgInitParam,
	.initPara =  initPara,
	.auto_close_time_set = 10,
};

static MyCtrlTitle ctrls_title[] = {
	{
        IDC_TITLE,
        MYTITLE_LEFT_EXIT,
        MYTITLE_RIGHT_SWICH,
        0,0,1024,40,
        "免扰设置",
        "",
        0xffffff, 0x333333FF,
        buttonTitleNotify,
    },
	{0},
};

static MyCtrlButton ctrls_button[] = {
	{0},
};
static FormBase* form_base = NULL;

static void enableAutoClose(void)
{
	reloadTimer(form_base->hDlg);
	Screen.setCurrent(form_base_priv.name);
	flag_timer_stop = 0;
}
static void saveStartTime(int time_buf)
{
	g_config.mute.start_time = time_buf;	
	ConfigSavePublic();
}
static void saveEndTime(int time_buf)
{
	g_config.mute.end_time = time_buf;	
	ConfigSavePublic();
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief reloadTimer 切换是否连接Mute
 *
 * @param hwnd
 * @param on_off
 */
/* ---------------------------------------------------------------------------*/
static void reloadTimer(HWND hwnd)
{
    if (g_config.mute.state) {
		int i;
		SVITEMINFO svii;
		struct ScrollviewItem *plist = locoal_list;
		SendMessage (hScrollView, SVM_RESETCONTENT, 0, 0);
		for (i=0; plist->title[0] != 0; i++) {
			plist->index = i;
			svii.nItemHeight = 60;
			svii.addData = (DWORD)plist;
			svii.nItem = i;
			if (strcmp("开始时间",plist->title) == 0) {
				plist->time_buf = g_config.mute.start_time;
				sprintf(plist->text,"%d:%02d",plist->time_buf / 60,plist->time_buf % 60);
			} else if (strcmp("结束时间",plist->title) == 0) {
				plist->time_buf = g_config.mute.end_time;
				sprintf(plist->text,"%d:%02d",plist->time_buf / 60,plist->time_buf % 60);
			}
			SendMessage (hScrollView, SVM_ADDITEM, 0, (LPARAM)&svii);
			SendMessage (hScrollView, SVM_SETITEMADDDATA, i, (DWORD)plist);
			plist++;
		}
        ShowWindow(GetDlgItem(hwnd,IDC_STATIC_IMG_WARNING),SW_HIDE);
        ShowWindow(GetDlgItem(hwnd,IDC_STATIC_TEXT_WARNING),SW_HIDE);
        ShowWindow(GetDlgItem(hwnd,IDC_SCROLLVIEW),SW_SHOWNORMAL);
    } else {
        ShowWindow(GetDlgItem(hwnd,IDC_STATIC_IMG_WARNING),SW_SHOWNORMAL);
        ShowWindow(GetDlgItem(hwnd,IDC_STATIC_TEXT_WARNING),SW_SHOWNORMAL);
        ShowWindow(GetDlgItem(hwnd,IDC_SCROLLVIEW),SW_HIDE);
    }
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief saveConfigCallback 切换Mute开关，保存配置后回调函数
 */
/* ---------------------------------------------------------------------------*/
static void saveConfigCallback(void)
{
	reloadTimer(form_base->hDlg);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief buttonTitleNotify 标题按钮
 *
 * @param hwnd
 * @param id
 * @param nc
 * @param add_data
 */
/* ---------------------------------------------------------------------------*/
static void buttonTitleNotify(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc == MYTITLE_BUTTON_EXIT) {
		ShowWindow(GetParent(hwnd),SW_HIDE);
	}
    else if (nc == MYTITLE_BUTTON_SWICH) {
        g_config.mute.state = add_data;
		ConfigSavePublicCallback(saveConfigCallback);
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
	if (nc != SVN_CLICKED)
		return;
	int idx = SendMessage (hScrollView, SVM_GETCURSEL, 0, 0);
	struct ScrollviewItem *plist;
	plist = (struct ScrollviewItem *)SendMessage (hScrollView, SVM_GETITEMADDDATA, idx, 0);

	if (!plist)
		return;
	if (!plist->callback)
		return;
	flag_timer_stop = 1;
	plist->callback(hwnd,enableAutoClose,plist->time_buf,plist->saveCallback);
}

void formSettingMuteLoadBmp(void)
{
    if (bmp_load_finished == 1)
        return;

	printf("[%s]\n", __FUNCTION__);
    bmpsLoad(bmp_load);
    bmp_load_finished = 1;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myDrawItem 列表自定义绘图函数
 *
 * @param hWnd
 * @param hsvi
 * @param hdc
 * @param rcDraw
 */
/* ---------------------------------------------------------------------------*/
static void myDrawItem (HWND hWnd, HSVITEM hsvi, HDC hdc, RECT *rcDraw)
{
#define FILL_BMP_STRUCT(left,top,img)  \
	FillBoxWithBitmap(hdc,left, top,img.bmWidth,img.bmHeight,&img)

#define DRAW_TABLE(rc,offset,color)  \
	do { \
		SetPenColor (hdc, color); \
		if (p_item->index) { \
			MoveTo (hdc, rc->left + offset, rc->top); \
			LineTo (hdc, rc->right,rc->top); \
		} \
		MoveTo (hdc, rc->left + offset, rc->bottom); \
		LineTo (hdc, rc->right,rc->bottom); \
	} while (0)

	struct ScrollviewItem *p_item = (struct ScrollviewItem *)scrollview_get_item_adddata (hsvi);
	SetBkMode (hdc, BM_TRANSPARENT);
	SetTextColor (hdc, PIXEL_lightwhite);
	SelectFont (hdc, font20);
	if (p_item->callback)
		FILL_BMP_STRUCT(rcDraw->left + 968,rcDraw->top + 15,bmp_enter);
	// 绘制表格
	DRAW_TABLE(rcDraw,0,0xCCCCCC);
	// 输出文字
	TextOut (hdc, rcDraw->left + 30, rcDraw->top + 15, p_item->title);
	RECT rc;
	memcpy(&rc,rcDraw,sizeof(RECT));
	rc.left += 512;
	rc.right -= 70;
	SetTextColor (hdc, 0xCCCCCC);
	DrawText (hdc,p_item->text, -1, &rc,
			DT_VCENTER | DT_RIGHT | DT_WORDBREAK  | DT_SINGLELINE);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief MuteLoadData 重新加载Mute列表
 *
 * @param aps
 * @param ap_cnt
 */
/* ---------------------------------------------------------------------------*/
static void MuteLoadData(void *aps,int ap_cnt)
{
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
	SendMessage(GetDlgItem(hDlg,IDC_TITLE),
            MSG_MYTITLE_SET_SWICH, (WPARAM)g_config.mute.state, 0);
    reloadTimer(form_base->hDlg);
}

/* ----------------------------------------------------------------*/
/**
 * @brief formSettingMuteProc 窗口回调函数
 *
 * @param hDlg
 * @param message
 * @param wParam
 * @param lParam
 *
 * @return
 */
/* ----------------------------------------------------------------*/
static int formSettingMuteProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    HDC         hdc;
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

int createFormSettingMute(HWND hMainWnd,void (*callback)(void))
{
	HWND Form = Screen.Find(form_base_priv.name);
	if(Form) {
		Screen.setCurrent(form_base_priv.name);
        reloadTimer(form_base->hDlg);
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

