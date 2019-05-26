/*
 * =============================================================================
 *
 *       Filename:  form_Password.c
 *
 *    Description:  密码输入界面
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
#include <stdio.h>
#include <string.h>
#include "externfunc.h"
#include "screen.h"

#include "my_button.h"
#include "my_title.h"

#include "form_base.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/
static int formPasswordProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam);
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
    FOMR_TYPE_PASSWORD,
    FOMR_TYPE_ACCOUNT,
};
enum {
	IDC_TIMER_1S = IDC_FORM_PASSWORD_STATR,
	IDC_BUTTON_WIFI,
	IDC_EDIT_PASSWORD,

	IDC_TITLE,
};

struct Keyboards {
	char *func[3];
	int state;
	RECT rc;
	BITMAP *bmp_nor;
	BITMAP *bmp_pre;
};
struct KeyboardsData {
	int click_x,click_y;
	int index;
	int key_type;
};
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static BITMAP bmp_key_nor;
static BITMAP bmp_key_pre;
static BITMAP bmp_key_delete_nor;
static BITMAP bmp_key_delete_pre;
static BITMAP bmp_key_abc_nor;
static BITMAP bmp_key_abc_pre;

static int bmp_load_finished = 0;
static int flag_timer_stop = 0;
struct KeyboardsData kb_data;
static int re_paint_abc = 0;
static int form_password_type = FOMR_TYPE_PASSWORD;
static char g_account[64] = {0};
static void (*getPasswordProc)(char *account,char *password);

static BmpLocation bmp_load[] = {
	{&bmp_key_nor,BMP_LOCAL_PATH"Key 1.png"},
	{&bmp_key_pre,BMP_LOCAL_PATH"Key 6_pre.png"},
	{&bmp_key_delete_nor,BMP_LOCAL_PATH"Key 删除.png"},
	{&bmp_key_delete_pre,BMP_LOCAL_PATH"Key 删除_pre.png"},
	{&bmp_key_abc_nor,BMP_LOCAL_PATH"Key abc_1.png"},
	{&bmp_key_abc_pre,BMP_LOCAL_PATH"Key abc_1_pre.png"},
    {NULL},
};

static MY_CTRLDATA ChildCtrls [] = {
    EDIT(0,91,1024,60,IDC_EDIT_PASSWORD,"",&font22,0xffffff),
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
	.name = "Fpsd",
	.idc_timer = IDC_TIMER_1S,
	.dlgProc = formPasswordProc,
	.dlgInitParam = &DlgInitParam,
	.initPara =  initPara,
};

static struct Keyboards keys[] = {
	{"0","0","["},
	{"1","1","]"},
	{"2","2","{"},
	{"3","3","}"},
	{"4","4","#"},
	{"5","5","%"},
	{"6","6","^"},
	{"7","7","*"},
	{"8","8","+"},
	{"9","9","="},

	{"q","Q","_"},
	{"w","W","\\"},
	{"e","E","|"},
	{"r","R","~"},
	{"t","T","<"},
	{"y","Y",">"},
	{"u","U","U"},
	{"i","I","I"},
	{"o","O","¥"},
	{"p","P","·"},

	{"ABC","abc","-"},
	{"a","A","/"},
	{"s","S",":"},
	{"d","D",";"},
	{"f","F","("},
	{"g","G",")"},
	{"h","H","$"},
	{"j","J","&"},
	{"k","K","@"},
	{"l","L","\""},

	{"#+=","#+=","abc"},
	{"z","Z","abc"},
	{"x","X","'"},
	{"c","C","."},
	{"v","V",","},
	{"b","B","?"},
	{"n","N","!"},
	{"m","M","'"},
	{"\b","\b","\b"},
	{NULL},
};

static MyCtrlButton ctrls_button[] = {
	{0},
};
static MyCtrlTitle ctrls_title[] = {
	{
        IDC_TITLE,
        MYTITLE_LEFT_EXIT,
        MYTITLE_RIGHT_TEXT,
        0,0,1024,40,
        "输入密码",
        "确定",
        0xffffff, 0x333333FF,
        buttonNotify,
    },
	{0},
};

static FormBase* form_base = NULL;

static void enableAutoClose(void)
{
	flag_timer_stop = 0;
}

static void updateTitle(void)
{
    if (form_password_type == FOMR_TYPE_PASSWORD) {
        SendMessage(GetDlgItem(form_base->hDlg,IDC_TITLE),
                MSG_MYTITLE_SET_TITLE,(WPARAM)"输入密码",0);
    } else if (form_password_type == FOMR_TYPE_ACCOUNT) {
        SendMessage(GetDlgItem(form_base->hDlg,IDC_TITLE),
                MSG_MYTITLE_SET_TITLE,(WPARAM)"输入账号",0);
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
    else if (nc == MYTITLE_BUTTON_TEXT) {
		char text[64];
		int ret = SendMessage(GetDlgItem(GetParent(hwnd),IDC_EDIT_PASSWORD),
                MSG_GETTEXT,64,(LPARAM)text);
        if (form_password_type == FOMR_TYPE_ACCOUNT) {
            form_password_type = FOMR_TYPE_PASSWORD;
            strcpy(g_account,text);
            updateTitle();
        } else {
            if (getPasswordProc)
                getPasswordProc(g_account,text);
            ShowWindow(GetParent(hwnd),SW_HIDE);
        }
    }
	SendMessage(GetDlgItem(GetParent(hwnd),IDC_EDIT_PASSWORD),MSG_SETTEXT,0,(LPARAM)"");
}
static void keboardNotify(HWND hwnd, int click_type,int index)
{
	static int largeabc_index = 0; // 记录下需要改变的图标下标
	if (index < 0) {
		if (keys[kb_data.index].state == BN_PUSHED
				&& click_type == BN_UNPUSHED ) {
			keys[kb_data.index].state = BN_UNPUSHED;
			InvalidateRect (hwnd, &keys[kb_data.index].rc, FALSE);
		}
		return;
	}
	if (click_type == BN_PUSHED) {
		keys[index].state = BN_PUSHED;
		InvalidateRect (hwnd, &keys[index].rc, FALSE);
		return;
	}
    RECT rc ;
	keys[index].state = BN_UNPUSHED;
	rc.left = 0;
	rc.right = 1024;
	rc.top = 190;
	rc.bottom = 600;
	if (strcmp(keys[index].func[kb_data.key_type],"ABC") == 0) {
		kb_data.key_type = 1;
		InvalidateRect (hwnd, &rc, FALSE);
	} else if (strcmp(keys[index].func[kb_data.key_type],"abc") == 0) {
		if (kb_data.key_type == 2)
			re_paint_abc = 1;

		kb_data.key_type = 0;
		if (largeabc_index != 0) {
			keys[largeabc_index].bmp_nor = &bmp_key_nor;
			keys[largeabc_index].bmp_pre = &bmp_key_pre;
			keys[largeabc_index].rc.right = keys[largeabc_index].rc.left + keys[largeabc_index].bmp_nor->bmWidth;
			keys[largeabc_index].rc.bottom = keys[largeabc_index].rc.top + keys[largeabc_index].bmp_pre->bmHeight;
			keys[largeabc_index+1].bmp_nor = &bmp_key_nor;
			keys[largeabc_index+1].bmp_pre = &bmp_key_pre;
			keys[largeabc_index+1].rc.right = keys[largeabc_index+1].rc.left + keys[largeabc_index+1].bmp_nor->bmWidth;
			keys[largeabc_index+1].rc.bottom = keys[largeabc_index+1].rc.top + keys[largeabc_index+1].bmp_pre->bmHeight;
		}
		InvalidateRect (hwnd, &rc, FALSE);
	} else if (strcmp(keys[index].func[kb_data.key_type],"#+=") == 0) {
		largeabc_index = index;
		kb_data.key_type = 2;
		// 针对acb图标变化的情况，修改图片与刷新范围
		keys[largeabc_index].bmp_nor = &bmp_key_abc_nor;
		keys[largeabc_index].bmp_pre = &bmp_key_abc_pre;
		keys[largeabc_index].rc.right = keys[largeabc_index].rc.left + keys[largeabc_index].bmp_nor->bmWidth;
		keys[largeabc_index].rc.bottom = keys[largeabc_index].rc.top + keys[largeabc_index].bmp_pre->bmHeight;
		keys[largeabc_index+1].bmp_nor = NULL;
		keys[largeabc_index+1].bmp_pre = NULL;
		keys[largeabc_index+1].rc.right = keys[largeabc_index+1].rc.left;
		keys[largeabc_index+1].rc.bottom = keys[largeabc_index+1].rc.top;
		InvalidateRect (hwnd, &rc, FALSE);
	} else {
		SendMessage(GetDlgItem(hwnd,IDC_EDIT_PASSWORD),MSG_CHAR,keys[index].func[kb_data.key_type][0],0);
		InvalidateRect (hwnd, &keys[index].rc, FALSE);
	}
}

void formPasswordLoadBmp(void)
{
    if (bmp_load_finished == 1)
        return;

	printf("[%s]\n", __FUNCTION__);
    bmpsLoad(bmp_load);
    my_button->bmpsLoad(ctrls_button,BMP_LOCAL_PATH);
    bmp_load_finished = 1;
}

static void initKeyboard(void)
{
	int x_start = 15,y_start = 196,w = bmp_key_nor.bmWidth + 21, h = bmp_key_nor.bmHeight + 22;
	int i,j = -1;
	for (i=0; keys[i].func[0] != NULL; i++) {
		if (i % 10 == 0) {
			j++;
			keys[i].rc.left = x_start;
			keys[i].rc.top = y_start + h * j;
		} else {
			keys[i].rc.left = x_start + (i % 10) * w;
			keys[i].rc.top = y_start + h * j;
		}
		keys[i].bmp_nor = &bmp_key_nor;
		keys[i].bmp_pre = &bmp_key_pre;
		keys[i].rc.right = keys[i].rc.left + keys[i].bmp_nor->bmWidth;
		keys[i].rc.bottom = keys[i].rc.top + keys[i].bmp_pre->bmHeight;
		keys[i].state = BN_UNPUSHED;
	}
	// 删除键用其他的图片
	keys[i-1].bmp_nor = &bmp_key_delete_nor;
	keys[i-1].bmp_pre = &bmp_key_delete_pre;
	keys[i-1].rc.right = keys[i-1].rc.left + keys[i-1].bmp_nor->bmWidth;
	keys[i-1].rc.bottom = keys[i-1].rc.top + keys[i-1].bmp_pre->bmHeight;
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
	initKeyboard();
    updateTitle();
}

static void paint(HWND hWnd,HDC hdc)
{
#define FILL_BMP_STRUCT(rc,img)  rc.left, rc.top,img->bmWidth,img->bmHeight,img

	int i;
	SetPenColor (hdc, 0x12345678);
	MoveTo (hdc, 0, 90);
	LineTo (hdc, 1024,90);
	MoveTo (hdc, 0, 151);
	LineTo (hdc, 1024,151);
	SetTextColor(hdc,COLOR_lightwhite);
	SetBkMode(hdc,BM_TRANSPARENT);
	SelectFont (hdc, font20);
	if (re_paint_abc) {
		re_paint_abc = 0;
		SetBrushColor (hdc,0);
		FillBox (hdc, 16,502,180,80);
	}
	for (i=0; keys[i].func[0] != NULL; i++) {
		if (keys[i].state == BN_UNPUSHED) {
			if (keys[i].bmp_nor)
				FillBoxWithBitmap(hdc,FILL_BMP_STRUCT(keys[i].rc,keys[i].bmp_nor));
		} else if (keys[i].state == BN_PUSHED) {
			if (keys[i].bmp_pre)
				FillBoxWithBitmap(hdc,FILL_BMP_STRUCT(keys[i].rc,keys[i].bmp_pre));
		}
		if (keys[i].func[kb_data.key_type])
			DrawText (hdc,keys[i].func[kb_data.key_type], -1, &keys[i].rc,
					DT_CENTER | DT_VCENTER | DT_WORDBREAK  | DT_SINGLELINE);
	}
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief clickInButton 判断触摸坐标是否在按钮内
 *
 * @param pInfo
 * @param x
 * @param y
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int clickInButton(int x,int y)
{
    RECT *rc ;
	int i;
	for (i=0; keys[i].func[0] != NULL; i++) {
		rc = &keys[i].rc;
        if (PtInRect (rc, x, y) && PtInRect (rc, kb_data.click_x,kb_data.click_y))
            return i;
	}
    return -1;
}
/* ----------------------------------------------------------------*/
/**
 * @brief formPasswordProc 窗口回调函数
 *
 * @param hDlg
 * @param message
 * @param wParam
 * @param lParam
 *
 * @return
 */
/* ----------------------------------------------------------------*/
static int formPasswordProc(HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
    HDC         hdc;
    switch(message) // 自定义消息
    {
		case MSG_TIMER:
			{
				if (flag_timer_stop)
					return 0;
			} break;
        case MSG_LBUTTONDOWN:
            {
                int x, y;
                x = LOSWORD(lParam);
                y = HISWORD(lParam);
                if (GetCapture () == hDlg)
                    break;

                SetCapture (hDlg);
                kb_data.click_x = x;
                kb_data.click_y = y ;
                kb_data.index = clickInButton(x,y);
				keboardNotify(hDlg,BN_PUSHED,kb_data.index);
            }
            break;
        case MSG_LBUTTONUP:
            {
                int x, y;
                if (GetCapture() != hDlg) {
                    // if(pInfo->state!=BUT_NORMAL) {
                        // pInfo->state = BUT_NORMAL;
                        // InvalidateRect (hwnd, NULL, TRUE);
                    // }
                    break;
                }
                ReleaseCapture ();
                x = LOSWORD(lParam);
                y = HISWORD(lParam);
                int index = clickInButton(x,y);
				keboardNotify(hDlg,BN_UNPUSHED,index);

            } break;
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

int createFormPassword(HWND hMainWnd,char *account,
        void (*callback)(void),
        void (*getPassword)(char *account,char *password))
{
	HWND Form = Screen.Find(form_base_priv.name);
	getPasswordProc = getPassword;
    memset(g_account,0,sizeof(g_account));
    if (account) {
        strcpy(g_account,account);
        form_password_type = FOMR_TYPE_PASSWORD;
    } else
        form_password_type = FOMR_TYPE_ACCOUNT;
	if(Form) {
        updateTitle();
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

