/*
 * =============================================================================
 *
 *       Filename:  form_setting_CaptureVideo.c
 *
 *    Description:  CaptureVideo设置界面
 *
 *        Version:  1.0
 *        Created:  2019-12-05 23:32:41
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
#include "my_face.h"

#include "form_base.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static int formSettingCaptureVideoProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
static void initPara(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
static void loadCaptureVideoData(void);

static void buttonNotify(HWND hwnd, int id, int nc, DWORD add_data);
static int buttonSwitchCaptureNotify(HWND hwnd,void (*callback)(void));
static int buttonSwitchVideoNotify(HWND hwnd,void (*callback)(void));

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
	IDC_TIMER_1S = IDC_FORM_LOCAL_STATR,
	IDC_STATIC_IMG_WARNING,
	IDC_STATIC_TEXT_WARNING,

	IDC_SCROLLVIEW,
	IDC_BUTTON_NETX,
	IDC_BUTTON_PREV,

	IDC_TITLE,
};

struct ScrollviewItem {
	char title[32]; // 左边标题
	char text[32];  // 右边文字
	int (*callback)(HWND,void (*callback)(void)); // 点击回调函数
	int index;  // 元素位置
	int item_type; // 0标准 1开关
	int switch_state; // 当item_type为1时，此变量有效,0关闭，1开启
	int changed; // 当开关变化后为1，否则为0
};

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static BITMAP bmp_enter; // 进入
static BITMAP image_swich_off;// 关闭状态图片
static BITMAP image_swich_on;	// 打开状态图片
static HWND hScrollView;
static int bmp_load_finished = 0;
static int flag_timer_stop = 0;
// static struct ScrollviewItem *locoal_list;
// TEST
static struct ScrollviewItem locoal_list[] = {
	{"拍照","",buttonSwitchCaptureNotify},
	{"录像","",buttonSwitchVideoNotify},
	{"  ","",NULL},
	{"拍照设置","",NULL},
	{0},
};


static BmpLocation bmp_load[] = {
    {&bmp_enter,	BMP_LOCAL_PATH"ico_返回_1.png"},
	{&image_swich_off,BMP_LOCAL_PATH"ico_check_nor.png"},
	{&image_swich_on, BMP_LOCAL_PATH"ico_check_pre.png"},
    {NULL},
};

static MY_CTRLDATA ChildCtrls [] = {
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
	.name = "FsetCaptureVideo",
	.idc_timer = IDC_TIMER_1S,
	.dlgProc = formSettingCaptureVideoProc,
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
        MYTITLE_RIGHT_NULL,
        0,0,1024,40,
        "拍照录像设置",
        "",
        0xffffff, 0x333333FF,
        buttonNotify,
    },
	{0},
};

static FormBase* form_base = NULL;

static void enableAutoClose(void)
{
	Screen.setCurrent(form_base_priv.name);
	flag_timer_stop = 0;	
	loadCaptureVideoData();
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
}
static int buttonSwitchCaptureNotify(HWND hwnd,void (*callback)(void))
{
	ConfigSavePublic();
	loadCaptureVideoData();
}
static int buttonSwitchVideoNotify(HWND hwnd,void (*callback)(void))
{
	
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
	if (plist->item_type == 0) {
		flag_timer_stop = 1;
		plist->callback(hwnd,enableAutoClose);
	} else if (plist->item_type == 1){
		plist->switch_state ^= 1;
		plist->callback(plist->switch_state,NULL);
		InvalidateRect (hwnd, NULL, TRUE);
	}
}

void formSettingCaptureVideoLoadBmp(void)
{
    if (bmp_load_finished == 1)
        return;

	printf("[%s]\n", __FUNCTION__);
    bmpsLoad(bmp_load);
    bmp_load_finished = 1;
}
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
	if (p_item->callback && p_item->item_type == 0)
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
	if (p_item->item_type == 1) {
		rc.left += 512;
		rc.right -= 70;
		if (p_item->switch_state) {
			if (p_item->changed) {
				rc.left -= 252;
				rc.right += 412;
				DrawText (hdc,"打开(需等待重启生效)", -1, &rc,
						DT_VCENTER | DT_LEFT | DT_WORDBREAK  | DT_SINGLELINE);
			} else
				DrawText (hdc,"打开", -1, &rc,
						DT_VCENTER | DT_LEFT | DT_WORDBREAK  | DT_SINGLELINE);
			FILL_BMP_STRUCT(rcDraw->left + 968,rcDraw->top + 20,image_swich_on);
		} else {
			DrawText (hdc,"关闭", -1, &rc,
					DT_VCENTER | DT_LEFT | DT_WORDBREAK  | DT_SINGLELINE);
			FILL_BMP_STRUCT(rcDraw->left + 968,rcDraw->top + 20,image_swich_off);
		}
	}
}

static void loadCaptureVideoData(void)
{
	int i;
	SVITEMINFO svii;
	struct ScrollviewItem *plist = locoal_list;
    SendMessage (hScrollView, SVM_RESETCONTENT, 0, 0);
	for (i=0; plist->title[0] != 0; i++) {
		plist->index = i;
		plist->item_type = 0;
		svii.nItemHeight = 60;
		svii.addData = (DWORD)plist;
		svii.nItem = i;
		if (strcmp("设置",plist->title) == 0) {
			if (g_config.cap_doorbell.type == 0) {
				sprintf(plist->text,"拍照%d张",g_config.cap_doorbell.count);
			} else {
				sprintf(plist->text,"录像%d秒",g_config.cap_doorbell.timer);
			}
		}
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
	loadCaptureVideoData();
}

/* ----------------------------------------------------------------*/
/**
 * @brief formSettingCaptureVideoProc 窗口回调函数
 *
 * @param hDlg
 * @param message
 * @param wParam
 * @param lParam
 *
 * @return
 */
/* ----------------------------------------------------------------*/
static int formSettingCaptureVideoProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
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

int createFormSettingCaptureVideo(HWND hMainWnd,void (*callback)(void))
{
	HWND Form = Screen.Find(form_base_priv.name);
	if(Form) {
		Screen.setCurrent(form_base_priv.name);
		loadCaptureVideoData();
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

