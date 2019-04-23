/*
 * =====================================================================================
 *
 *       Filename:  MyStatus.c
 *
 *    Description:  自定义状态
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

#include "my_status.h"
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
static int myButtonControlProc (HWND hwnd, int message, WPARAM wParam, LPARAM lParam);

/* ----------------------------------------------------------------*
 *                        macro define
 *-----------------------------------------------------------------*/

/* ----------------------------------------------------------------*
 *                      variables define
 *-----------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*/
/**
 * @brief myButtonRegist 注册控件
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
BOOL myButtonRegist (void)
{
    WNDCLASS WndClass;

    WndClass.spClassName = CTRL_MYSTATUS;
    WndClass.dwStyle     = WS_NONE;
    WndClass.dwExStyle   = WS_EX_NONE;
    WndClass.hCursor     = GetSystemCursor (IDC_ARROW);
    WndClass.iBkColor    = 0;
    WndClass.WinProc     = myButtonControlProc;

    return RegisterWindowClass(&WndClass);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myButtonCleanUp 卸载控件
 */
/* ---------------------------------------------------------------------------*/
void myButtonCleanUp (void)
{
    UnregisterWindowClass(CTRL_MYSTATUS);
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
	RECT rcClient;
	PCONTROL    pCtrl;
	pCtrl = Control (hWnd);
	GetClientRect (hWnd, &rcClient);

	if (pCtrl->dwAddData2) {
		MyStatusCtrlInfo* pInfo = (MyStatusCtrlInfo*)(pCtrl->dwAddData2);
		if (pInfo->images + pInfo->level)
			FillBoxWithBitmap(hdc,
					rcClient.left,
					rcClient.top,
					(pInfo->images + pInfo->level)->bmWidth,
					(pInfo->images + pInfo->level)->bmHeight,
					pInfo->images + pInfo->level);
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myButtonControlProc 控件主回调函数
 *
 * @param hwnd
 * @param message
 * @param wParam
 * @param lParam
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
static int myButtonControlProc (HWND hwnd, int message, WPARAM wParam, LPARAM lParam)
{
    HDC         hdc;
    PCONTROL    pCtrl;
    DWORD       dwStyle;
    MyStatusCtrlInfo* pInfo;

	pCtrl = Control (hwnd);
	pInfo = (MyStatusCtrlInfo*)pCtrl->dwAddData2;
	dwStyle = GetWindowStyle (hwnd);

	switch (message) {
	case MSG_CREATE:
	{
		MyStatusCtrlInfo* data = (MyStatusCtrlInfo*)pCtrl->dwAddData;

		pInfo = (MyStatusCtrlInfo*) calloc (1, sizeof (MyStatusCtrlInfo));
		if (pInfo == NULL)
			return -1;
		memset(pInfo,0,sizeof(MyStatusCtrlInfo));
		pInfo->image_press = data->image_press;
		pInfo->image_normal = data->image_normal;
		pInfo->state = data->state;
		pInfo->select.mode = data->select.mode;
		pInfo->select.state = data->select.state;
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


    case MSG_MYSTATU_SET_LEVEL:
		pInfo->level = wParam;
		InvalidateRect (hwnd, NULL, FALSE);
		return 0;
	default:
		break;
    }

    return DefaultControlProc (hwnd, message, wParam, lParam);
}

HWND createMyStatus(HWND hWnd,MgCtrlButton *button)
{
	HWND hCtrl;
	MyStatusCtrlInfo pInfo;
	pInfo.image_normal = &button->image_normal;
	pInfo.image_press = &button->image_press;

	hCtrl = CreateWindowEx(CTRL_MYSTATUS,"",WS_VISIBLE|WS_CHILD,WS_EX_TRANSPARENT,
			button->idc,button->x,button->y,button->w,button->h, hWnd,(DWORD)&pInfo);
    return hCtrl;
}


