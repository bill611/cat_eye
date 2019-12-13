/*
 * =====================================================================================
 *
 *       Filename:  MyScroll.c
 *
 *    Description:  自定义按钮
 *
 *        Version:  1.0
 *        Created:  2015-12-09 16:22:37
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =====================================================================================
 */
/* ----------------------------------------------------------------*
 *                      include head files
 *-----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <memory.h>

#include "my_scroll.h"
#include "debug.h"
#include "cliprect.h"
#include "internals.h"
#include "ctrlclass.h"


/* ----------------------------------------------------------------*
 *                  extern variables declare
 *-----------------------------------------------------------------*/

/* ----------------------------------------------------------------*
 *                  internal functions declare
 *-----------------------------------------------------------------*/
static int myScrollControlProc (HWND hwnd, int message, WPARAM wParam, LPARAM lParam);

static void buttonTopPress(HWND hwnd, int id, int nc, DWORD add_data);
static void buttonBottomPress(HWND hwnd, int id, int nc, DWORD add_data);
/* ----------------------------------------------------------------*
 *                        macro define
 *-----------------------------------------------------------------*/
#define CTRL_NAME CTRL_MYSCROLL
#define BMP_LOCAL_PATH "setting/"
enum {
	IDC_BUTTON_TOP = 1,
	IDC_BUTTON_BOTTOM,
};
/* ----------------------------------------------------------------*
 *                      variables define
 *-----------------------------------------------------------------*/
MyControls *my_scroll;

static MyCtrlButton ctrls_button[] = {
	{IDC_BUTTON_TOP,	 MYBUTTON_TYPE_TWO_STATE|MYBUTTON_TYPE_TEXT_NULL,"top",0, 0,buttonTopPress},
	{IDC_BUTTON_BOTTOM,	 MYBUTTON_TYPE_TWO_STATE|MYBUTTON_TYPE_TEXT_NULL,"bottom",0,200,buttonBottomPress},
	{0},
};


/* ---------------------------------------------------------------------------*/
/**
 * @brief myScrollRegist 注册控件
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static BOOL myScrollRegist (void)
{
    WNDCLASS WndClass;

    WndClass.spClassName = CTRL_NAME;
    WndClass.dwStyle     = WS_NONE;
    WndClass.dwExStyle   = WS_EX_NONE;
    WndClass.hCursor     = GetSystemCursor (IDC_ARROW);
    WndClass.iBkColor    = 0;
    WndClass.WinProc     = myScrollControlProc;

    return RegisterWindowClass(&WndClass);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myScrollCleanUp 卸载控件
 */
/* ---------------------------------------------------------------------------*/
static void myScrollCleanUp (void)
{
    UnregisterWindowClass(CTRL_NAME);
}

static void buttonTopPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
    PCONTROL    pCtrl = Control (GetParent(hwnd));
	MyScrollCtrlInfo* pInfo = (MyScrollCtrlInfo*)(pCtrl->dwAddData2);
    int num = pInfo->rc_text[pInfo->index_center].num;
    if (num >= pInfo->index_end)
        SendMessage(GetParent(hwnd),MSG_SET_NUM,pInfo->index_start,0);
    else
        SendMessage(GetParent(hwnd),MSG_SET_NUM,++num,0);
}
static void buttonBottomPress(HWND hwnd, int id, int nc, DWORD add_data)
{
	if (nc != BN_CLICKED)
		return;
    PCONTROL    pCtrl = Control (GetParent(hwnd));
	MyScrollCtrlInfo* pInfo = (MyScrollCtrlInfo*)(pCtrl->dwAddData2);
    int num = pInfo->rc_text[pInfo->index_center].num;
    if (num <= pInfo->index_start)
        SendMessage(GetParent(hwnd),MSG_SET_NUM,pInfo->index_end,0);
    else
        SendMessage(GetParent(hwnd),MSG_SET_NUM,--num,0);
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief paint 主要绘图函数
 *
 * @param hWnd
 * @param hdc
 */
/* ---------------------------------------------------------------------------*/
static void paint(HWND hWnd,HDC hdc)
{
	RECT rc_bmp,*rc_text;
    PCONTROL    pCtrl;
    pCtrl = Control (hWnd);
	GetClientRect (hWnd, &rc_bmp);
    rc_text = &rc_bmp;

	MyScrollCtrlInfo* pInfo = (MyScrollCtrlInfo*)(pCtrl->dwAddData2);
	if (!pCtrl->dwAddData2)
		return;

	SetBkMode(hdc,BM_TRANSPARENT);
	SetTextColor(hdc,COLOR_lightwhite);
	SelectFont (hdc, pInfo->font);
	SetPenColor (hdc, 0xCCCCCC);
	int i;
    char buf[6] = {0};
	for (i=0; i<MYSCROLL_MAX_LINES; i++) {
		if (i != MYSCROLL_MAX_LINES - 1) {
			MoveTo (hdc, pInfo->rc_text[i].rc.left, pInfo->rc_text[i].rc.bottom);
			LineTo (hdc, pInfo->rc_text[i].rc.right, pInfo->rc_text[i].rc.bottom);
		}
        sprintf(buf,"%d",pInfo->rc_text[i].num);
        DrawText (hdc,buf, -1, &pInfo->rc_text[i].rc,
                DT_CENTER | DT_VCENTER | DT_WORDBREAK  | DT_SINGLELINE);
        if (i == pInfo->index_center) {
            DrawText (hdc,pInfo->text, -1, &pInfo->rc_text[i].rc,
                    DT_RIGHT | DT_VCENTER | DT_WORDBREAK  | DT_SINGLELINE);
        }
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myScrollControlProc 控件主回调函数
 *
 * @param hwnd
 * @param message
 * @param wParam
 * @param lParam
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int myScrollControlProc (HWND hwnd, int message, WPARAM wParam, LPARAM lParam)
{
	int i;
    HDC         hdc;
    PCONTROL    pCtrl;
    DWORD       dwStyle;
	RECT rc;
    MyScrollCtrlInfo* pInfo;

	pCtrl = Control (hwnd);
	pInfo = (MyScrollCtrlInfo*)pCtrl->dwAddData2;
	dwStyle = GetWindowStyle (hwnd);

	switch (message) {
	case MSG_CREATE:
	{
		MyScrollCtrlInfo* data = (MyScrollCtrlInfo*)pCtrl->dwAddData;

		pInfo = (MyScrollCtrlInfo*) calloc (1, sizeof (MyScrollCtrlInfo));
		if (pInfo == NULL)
			return -1;
		memset(pInfo,0,sizeof(MyScrollCtrlInfo));
		pInfo->flag = data->flag;
		pInfo->text = data->text;
		pInfo->font = data->font;
		pInfo->index_start = data->index_start;
		pInfo->index_end = data->index_end;
        pInfo->index_center = MYSCROLL_MAX_LINES/2;
		GetClientRect (hwnd, &rc);
		int text_hight = RECTH(rc) - (ctrls_button[0].image_press.bmHeight * 2);

		for (i=0; ctrls_button[i].idc != 0; i++) {
			ctrls_button[i].font = font22;
			ctrls_button[i].x = rc.left + (RECTW(rc) - ctrls_button[i].image_press.bmWidth) / 2;
			ctrls_button[i].y = rc.top + i*(ctrls_button[i].image_press.bmHeight + text_hight);
			createMyButton(hwnd,&ctrls_button[i]);
		}
		for (i=0; i<MYSCROLL_MAX_LINES; i++) {
			pInfo->rc_text[i].rc.left = rc.left;
			pInfo->rc_text[i].rc.right = rc.right;
			pInfo->rc_text[i].rc.top = rc.top + i*text_hight/MYSCROLL_MAX_LINES + ctrls_button[0].image_press.bmHeight;
			pInfo->rc_text[i].rc.bottom = pInfo->rc_text[i].rc.top + text_hight/MYSCROLL_MAX_LINES;
		}
		pCtrl->dwAddData2 = (DWORD)pInfo;
		return 0;
	}
	case MSG_DESTROY:
		free(pInfo);
		break;

    case MSG_PAINT:
        hdc = BeginPaint (hwnd);
		paint(hwnd,hdc);
        EndPaint (hwnd, hdc);
        return 0;
	case MSG_SET_NUM:
        {
            int i;
            pInfo->rc_text[pInfo->index_center].num = wParam;
            if (pInfo->index_center < 1)
                break;
            for (i=pInfo->index_center-1; i>=0; i--) {
                pInfo->rc_text[i].num = wParam - 1;
                if (pInfo->rc_text[i].num < pInfo->index_start) {
                    pInfo->rc_text[i].num =
                        pInfo->index_end - (pInfo->index_start - pInfo->rc_text[i].num) + 1;
                }
            }
            for (i=pInfo->index_center+1; i<MYSCROLL_MAX_LINES; i++) {
                pInfo->rc_text[i].num = wParam + 1;
                if (pInfo->index_end < pInfo->rc_text[i].num) {
                    pInfo->rc_text[i].num =
                        pInfo->index_start + (pInfo->rc_text[i].num - pInfo->index_end) - 1;
                }
            }
            InvalidateRect (hwnd, NULL, TRUE);
        } break;
	case MSG_GET_NUM:
        {
            return pInfo->rc_text[pInfo->index_center].num;
        } break;
	default:
		break;
	}

    return DefaultControlProc (hwnd, message, wParam, lParam);
}

/* ---------------------------------------------------------------------------*/
/**
  * @brief bmpsMainScrollLoad 加载主界面图片
  *
  * @param controls
**/
/* ---------------------------------------------------------------------------*/
static void myScrollBmpsLoad(void *ctrls,char *path)
{
    my_button->bmpsLoad(ctrls_button,BMP_LOCAL_PATH);
}
static void myScrollBmpsRelease(void *ctrls)
{
    my_button->bmpsRelease(ctrls_button);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief createMyScroll 创建单个皮肤按钮
 *
 * @param hWnd
 * @param id
 * @param x,y,w,h 坐标
 * @param image_normal 正常图片
 * @param image_press  按下图片
 * @param display 是否显示 0不显示 1显示
 * @param mode 是否为选择模式 0非选择 1选择
 * @param notif_proc   回调函数
 */
/* ---------------------------------------------------------------------------*/
HWND createMyScroll(HWND hWnd,MyCtrlScroll *ctrl)
{
	HWND hCtrl;
	MyScrollCtrlInfo pInfo;
	pInfo.flag = ctrl->flag;
	pInfo.text = ctrl->text;
    pInfo.font = ctrl->font;
    pInfo.index_start = ctrl->index_start;
    pInfo.index_end = ctrl->index_end;

    hCtrl = CreateWindowEx(CTRL_NAME,"",WS_VISIBLE|WS_CHILD,WS_EX_TRANSPARENT,
            ctrl->idc,ctrl->x,ctrl->y,ctrl->w,ctrl->h, hWnd,(DWORD)&pInfo);
    return hCtrl;
}

void initMyScroll(void)
{
	my_scroll = (MyControls *)malloc(sizeof(MyControls));
	my_scroll->regist = myScrollRegist;
	my_scroll->unregist = myScrollCleanUp;
	my_scroll->bmpsLoad = myScrollBmpsLoad;
	my_scroll->bmpsRelease = myScrollBmpsRelease;
}


