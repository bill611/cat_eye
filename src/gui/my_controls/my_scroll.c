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

/* ----------------------------------------------------------------*
 *                        macro define
 *-----------------------------------------------------------------*/
#define CTRL_NAME CTRL_MYSCROLL
/* ----------------------------------------------------------------*
 *                      variables define
 *-----------------------------------------------------------------*/
MyControls *my_scroll;

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
#if 1
#define FILL_BMP_STRUCT(rc,img)  rc.left, rc.top,img->bmWidth,img->bmHeight,img

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
	printf("%d,%d,%d,%d,%s\n",
			rc_text->left,
			rc_text->top,
			rc_text->right,
			rc_text->bottom,
			pInfo->text );
	DrawText (hdc,pInfo->text, -1, rc_text,
			DT_CENTER | DT_VCENTER | DT_WORDBREAK  | DT_SINGLELINE);
#endif
}

static void* threadMoveInterval(void *arg)
{

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
    HDC         hdc;
    PCONTROL    pCtrl;
    DWORD       dwStyle;
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

    case MSG_LBUTTONDOWN:
		{

		} break;
	case MSG_LBUTTONUP:
		{
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
}
static void myScrollBmpsRelease(void *ctrls)
{
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


