/*
 * =============================================================================
 *
 *       Filename:  form_monitor.c
 *
 *    Description:  设置界面
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

#include "sql_handle.h"
#include "protocol.h"
#include "my_video.h"
#include "form_video.h"
#include "form_base.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static int formMonitorProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
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

#define BMP_LOCAL_PATH "main/"
enum {
	IDC_TIMER_1S = IDC_FORM_MONITOR_STATR,
	IDC_BUTTON_EXIT,
	IDC_ICONVIEW,

	IDC_TITLE,
};

struct IconviewItem {
	char user_id[32];
	char nick_name[128];
};
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static HWND hIconView;
static BITMAP bmp_access_nor; // 背景
static BITMAP bmp_access_pre; // 背景

static int bmp_load_finished = 0;
static int flag_timer_stop = 0;
static struct IconviewItem item_data[16];

static BmpLocation bmp_load[] = {
	{&bmp_access_pre,BMP_LOCAL_PATH"ico_监视门口机_pre.png"},
	{&bmp_access_nor,BMP_LOCAL_PATH"ico_监视门口机_nor.png"},
    {NULL},
};

static MY_CTRLDATA ChildCtrls [] = {
    ICONVIEW(192,200,640,304,IDC_ICONVIEW),
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
	.name = "Fmonitor",
	.idc_timer = IDC_TIMER_1S,
	.dlgProc = formMonitorProc,
	.dlgInitParam = &DlgInitParam,
	.initPara =  initPara,
};

static MyCtrlButton ctrls_button[] = {
	{IDC_BUTTON_EXIT,MYBUTTON_TYPE_ONE_STATE|MYBUTTON_TYPE_TEXT_NULL,"关闭",974,20,buttonExitPress},
	{0},
};
static MyCtrlTitle ctrls_title[] = {
	{0},
};

static FormBase* form_base = NULL;

static void enableAutoClose(void)
{
	Screen.setCurrent(form_base_priv.name);
	flag_timer_stop = 0;
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

/* ---------------------------------------------------------------------------*/
/**
 * @brief paint 固定表头绘制函数
 *
 * @param hWnd
 * @param hdc
 */
/* ---------------------------------------------------------------------------*/
static void paint (HWND hWnd, HDC hdc)
{
	RECT rc_text;
	SetBrushColor (hdc,PIXEL_lightwhite);
	rc_text.left = 192;
	rc_text.right = 832;
	rc_text.top = 148;
	rc_text.bottom = 200;
	FillBox (hdc, rc_text.left,rc_text.top,RECTW(rc_text),RECTH(rc_text));

	RECT rcDraw;
	SetBkMode (hdc, BM_TRANSPARENT);
	SetTextColor (hdc, 0x10B7F5);
	SelectFont (hdc, font20);
	rcDraw.left = 206;
	rcDraw.top = 159;
	TextOut (hdc, rcDraw.left, rcDraw.top, "监视门口机");

	SetPenColor (hdc, 0xDDDDDD);
	MoveTo (hdc, 192,199); LineTo (hdc, 832,199);
	// MoveTo (hdc, 192,325); LineTo (hdc, 832,325);

	// MoveTo (hdc, 318,199); LineTo (hdc, 318,492);
	// MoveTo (hdc, 447,199); LineTo (hdc, 447,492);
	// MoveTo (hdc, 576,199); LineTo (hdc, 576,492);
	// MoveTo (hdc, 705,199); LineTo (hdc, 705,492);
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
static void myDrawItem (HWND hWnd, GHANDLE hsvi, HDC hdc, RECT *rcDraw)
{
#define FILL_BMP_STRUCT(left,top,img)  \
	FillBoxWithBitmap(hdc,left, top,img.bmWidth,img.bmHeight,&img)

	const char *label = (const char*)iconview_get_item_label (hsvi);

	SetBkMode (hdc, BM_TRANSPARENT);
	SelectFont (hdc, font20);

	int bmp_x = rcDraw->left + 45;//rcDraw->left + (rcDraw->right - rcDraw->left - bmp_access_pre.bmWidth) / 2;
	int bmp_y = rcDraw->top + 26;//rcDraw->top + (rcDraw->bottom - rcDraw->top - bmp_access_pre.bmHeight) / 2;
	if (iconview_is_item_hilight(hWnd, hsvi)) {
		SetTextColor (hdc, 0x10B7F5);
		FILL_BMP_STRUCT(bmp_x,bmp_y,bmp_access_pre);
	} else {
		SetTextColor (hdc, 0x999999);
		FILL_BMP_STRUCT(bmp_x,bmp_y,bmp_access_nor);
	}

	if (label) {
		RECT rcTxt = *rcDraw;
		rcTxt.top = rcDraw->top + 77 ;//rcTxt.bottom - GetWindowFont (hWnd)->size * 2;
		// rcTxt.left = rcTxt.left - (GetWindowFont (hWnd)->size) + 2;

		DrawText (hdc, label, -1, &rcTxt,  DT_CENTER | DT_VCENTER);
	}
	// SetPenColor (hdc, 0xCCCCCC);
	// MoveTo (hdc, rcDraw->left ,rcDraw->top);
	// LineTo (hdc, rcDraw->right,rcDraw->top);
	// MoveTo (hdc, rcDraw->left ,rcDraw->top);
	// LineTo (hdc, rcDraw->left,rcDraw->bottom);

	// MoveTo (hdc, rcDraw->left ,rcDraw->bottom);
	// LineTo (hdc, rcDraw->right,rcDraw->bottom);
	// MoveTo (hdc, rcDraw->right ,rcDraw->top);
	// LineTo (hdc, rcDraw->bottom,rcDraw->top);
}

static void iconviewNotify(HWND hwnd, int id, int nc, DWORD add_data)
{
    int idx = SendMessage (hIconView, SVM_GETCURSEL, 0, 0);
    char *user_id;
    user_id = (char *)SendMessage (hIconView, SVM_GETITEMADDDATA, idx, 0);

    if (user_id) {
		ShowWindow(hwnd,SW_HIDE);
		createFormVideo(0,FORM_VIDEO_TYPE_MONITOR,NULL,0); 
		my_video->videoCallOut(user_id);
    }
}
static void iconviewAddItem(int count,char *name,char *user_id)
{
	IVITEMINFO ivii;
	memset (&ivii, 0, sizeof(IVITEMINFO));
	ivii.bmp = &bmp_access_nor;
	ivii.nItem = count;
	ivii.label = name;
	ivii.addData = (DWORD)user_id;
	SendMessage (hIconView, IVM_ADDITEM, 0, (LPARAM)&ivii);
}
void formMonitorLoadBmp(void)
{
    if (bmp_load_finished == 1)
        return;

	printf("[%s]\n", __FUNCTION__);
    bmpsLoad(bmp_load);
    my_button->bmpsLoad(ctrls_button,BMP_LOCAL_PATH);
    bmp_load_finished = 1;
}

static void loadIconviewData(void)
{
	int i;
	memset(item_data,0,sizeof(struct IconviewItem));
	int user_num = sqlGetUserInfoUseScopeStart(DEV_TYPE_ENTRANCEMACHINE);
	SendMessage (hIconView, IVM_RESETCONTENT, 0, 0);
	for (i=0; i<user_num && i<16; i++) {
		sqlGetUserInfosUseScope(item_data[i].user_id,item_data[i].nick_name);
		iconviewAddItem(i,item_data[i].nick_name,item_data[i].user_id);
	}
	sqlGetUserInfoEnd();
	user_num = sqlGetUserInfoUseScopeStart(DEV_TYPE_HOUSEENTRANCEMACHINE);
	for (; i<user_num && i<16; i++) {
		sqlGetUserInfosUseScope(item_data[i].user_id,item_data[i].nick_name);
		iconviewAddItem(i,item_data[i].nick_name,item_data[i].user_id);
	}
	sqlGetUserInfoEnd();
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
	hIconView = GetDlgItem (hDlg, IDC_ICONVIEW);
	SetWindowBkColor (hIconView, PIXEL_lightwhite);
	SendMessage (hIconView, IVM_SETITEMDRAW, 0, (LPARAM)myDrawItem);
	SendMessage (hIconView, IVM_SETITEMSIZE, 123, 152);
	RECT rcMargin;
	memset(&rcMargin,0,sizeof(RECT));
	SendMessage (hIconView, IVM_SETMARGINS, 0, (LPARAM)&rcMargin);
	loadIconviewData();
}

/* ----------------------------------------------------------------*/
/**
 * @brief formMonitorProc 窗口回调函数
 *
 * @param hDlg
 * @param message
 * @param wParam
 * @param lParam
 *
 * @return
 */
/* ----------------------------------------------------------------*/
static int formMonitorProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    HDC         hdc;
    switch(message) // 自定义消息
    {
		case MSG_TIMER:
			{
				if (flag_timer_stop)
					return 0;
			} break;

		case MSG_PAINT:
			hdc = BeginPaint (hDlg);
			paint(hDlg,hdc);
			EndPaint (hDlg, hdc);
			return 0;

		case MSG_COMMAND:
			{
				int id = LOWORD (wParam);
				int code = HIWORD (wParam);
				if (code == SVN_CLICKED)
					iconviewNotify(hDlg,id,code,0);
				break;
			}
		case MSG_ERASEBKGND:
			{
				drawBackground(hDlg,
						(HDC)wParam,
						(const RECT*)lParam, NULL,0x0);
			} return 0;


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

int createFormMonitor(HWND hMainWnd,void (*callback)(void))
{
	HWND Form = Screen.Find(form_base_priv.name);
	if(Form) {
		Screen.setCurrent(form_base_priv.name);
		loadIconviewData();
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

