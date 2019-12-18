/*
 * =============================================================================
 *
 *       Filename:  form_setting_Local.c
 *
 *    Description:  Local设置界面
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
extern int createFormSettingStore(HWND hMainWnd,void (*callback)(void));
extern int createFormSettingQrcode(HWND hMainWnd,void (*callback)(void));
extern int createFormSettingUpdate(HWND hMainWnd,void (*callback)(void));
extern int createFormSettingFace(HWND hMainWnd,void (*callback)(void));
extern int topMsgFactory(HWND hMainWnd,void (*callback)(void));
/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static int formSettingLocalProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
static void initPara(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);

static void buttonNotify(HWND hwnd, int id, int nc, DWORD add_data);
static void loadLocalData(void);

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

enum {
	SCROLLVIEW_ITEM_TYPE_TITLE,
	SCROLLVIEW_ITEM_TYPE_LIST,
};
struct ScrollviewItem {
	char title[32]; // 左边标题
	char text[32];  // 右边文字
	int (*callback)(HWND,void (*callback)(void)); // 点击回调函数
	int index;  // 元素位置
};

struct MemData {
	char total[32];
	char residue[32];
	char used[32];
};
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static HWND hScrollView;
static int bmp_load_finished = 0;
static int flag_timer_stop = 0;
struct MemData mem_data;
// static struct ScrollviewItem *locoal_list;
// TEST
static struct ScrollviewItem locoal_list[] = {
	{"设备型号",DEVICE_TYPE,NULL},
	{"软件版本",DEVICE_SVERSION"/"DEVICE_KVERSION,createFormSettingUpdate},
	{"二维码",  "扫描添加设备",createFormSettingQrcode},
	// {"本地存储","",NULL},
	{"人脸识别","",createFormSettingFace},
	{"恢复出厂设置","",topMsgFactory},
	{0},
};
static BITMAP bmp_warning; // 警告
static BITMAP bmp_security; // 加密
static BITMAP bmp_select; // 选中
static BITMAP bmp_enter; // 进入


static BmpLocation bmp_load[] = {
    {&bmp_enter,	BMP_LOCAL_PATH"ico_返回_1.png"},
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
	.name = "FsetLocal",
	.idc_timer = IDC_TIMER_1S,
	.dlgProc = formSettingLocalProc,
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
        "本机设置",
        "",
        0xffffff, 0x333333FF,
        buttonNotify,
    },
	{0},
};

static FormBase* form_base = NULL;

static void enableAutoClose(void)
{
	loadLocalData();
	Screen.setCurrent(form_base_priv.name);
	flag_timer_stop = 0;	
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
			if (plist->callback) {
				flag_timer_stop = 1;
				plist->callback(hwnd,enableAutoClose);
			}
		}
	}
}

void formSettingLocalLoadBmp(void)
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

static void loadLocalData(void)
{
	int i;
	SVITEMINFO svii;
	struct ScrollviewItem *plist = locoal_list;
    SendMessage (hScrollView, SVM_RESETCONTENT, 0, 0);
	for (i=0; plist->title[0] != 0; i++) {
		plist->index = i;
		svii.nItemHeight = 60;
		svii.addData = (DWORD)plist;
		svii.nItem = i;
		if (strcmp("本地存储",plist->title) == 0) {
			if (checkSD() == -1) {
				strcpy(plist->text,"未检测到SD卡");
				plist->callback = NULL;
			} else {
				memset(&mem_data,0,sizeof(mem_data));
				getSdMem(mem_data.total,mem_data.residue,mem_data.used);
				sprintf(plist->text,"剩余:%s",mem_data.residue);
				plist->callback = createFormSettingStore;
			}
		}else if (strcmp("软件版本",plist->title) == 0) {
			sprintf(plist->text,"%s/%s/%s",DEVICE_SVERSION,DEVICE_KVERSION,g_config.s_version);
		}else if (strcmp("人脸识别",plist->title) == 0) {
			if (strcmp(g_config.f_license,"0") == 0) {
				sprintf(plist->text,"未授权");
			} else {
				sprintf(plist->text,"已授权");
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
	loadLocalData();
}

/* ----------------------------------------------------------------*/
/**
 * @brief formSettingLocalProc 窗口回调函数
 *
 * @param hDlg
 * @param message
 * @param wParam
 * @param lParam
 *
 * @return
 */
/* ----------------------------------------------------------------*/
static int formSettingLocalProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
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

int createFormSettingLocal(HWND hMainWnd,void (*callback)(void))
{
	HWND Form = Screen.Find(form_base_priv.name);
	if(Form) {
		Screen.setCurrent(form_base_priv.name);
		loadLocalData();
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

