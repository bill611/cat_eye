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
static int myctrlControlProc (HWND hwnd, int message, WPARAM wParam, LPARAM lParam);

/* ----------------------------------------------------------------*
 *                        macro define
 *-----------------------------------------------------------------*/
#define CTRL_NAME CTRL_MYSTATUS

/* ----------------------------------------------------------------*
 *                      variables define
 *-----------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*/
/**
 * @brief myStatusRegist 注册控件
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
BOOL myStatusRegist (void)
{
    WNDCLASS WndClass;

    WndClass.spClassName = CTRL_NAME;
    WndClass.dwStyle     = WS_NONE;
    WndClass.dwExStyle   = WS_EX_NONE;
    WndClass.hCursor     = GetSystemCursor (IDC_ARROW);
    WndClass.iBkColor    = 0;
    WndClass.WinProc     = myctrlControlProc;

    return RegisterWindowClass(&WndClass);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myStatusCleanUp 卸载控件
 */
/* ---------------------------------------------------------------------------*/
void myStatusCleanUp (void)
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
 * @brief myctrlControlProc 控件主回调函数
 *
 * @param hwnd
 * @param message
 * @param wParam
 * @param lParam
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
static int myctrlControlProc (HWND hwnd, int message, WPARAM wParam, LPARAM lParam)
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
		pInfo->images = data->images;
		pInfo->level = data->level;
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

void myStatusBmpsLoad(MyCtrlStatus *ctrls,char *local_path)
{
    int i;
    char image_path[128] = {0};
    ctrls->images = (BITMAP *)calloc(ctrls->total_level,sizeof(BITMAP));
    for (i=0; i<ctrls->total_level; i++) {
        sprintf(image_path,"%s%s-%d.png",local_path,ctrls->img_name,i);
        bmpLoad(ctrls->images + i, image_path);
    }
}

HWND createMyStatus(HWND hWnd,MyCtrlStatus *ctrl)
{
	HWND hCtrl;
	MyStatusCtrlInfo pInfo;
    memset(&pInfo,0,sizeof(MyStatusCtrlInfo));
    pInfo.total_level = ctrl->total_level;
    pInfo.images = ctrl->images;

	hCtrl = CreateWindowEx(CTRL_NAME,"",WS_VISIBLE|WS_CHILD,WS_EX_TRANSPARENT,
			ctrl->idc,ctrl->x,ctrl->y,ctrl->w,ctrl->h, hWnd,(DWORD)&pInfo);
    return hCtrl;
}


