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
static BOOL myStatusRegist (void)
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
static void myStatusCleanUp (void)
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
		BITMAP *bmp = pInfo->images + pInfo->level; 
		if (bmp)
			FillBoxWithBitmap(hdc,
					rcClient.left, rcClient.top,
					bmp->bmWidth, bmp->bmHeight, bmp);
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


    case MSG_MYSTATUS_SET_LEVEL:
        pInfo->level = wParam;
        InvalidateRect (hwnd, NULL, FALSE);
        return 0;
	default:
		break;
    }

    return DefaultControlProc (hwnd, message, wParam, lParam);
}

static void myStatusBmpsLoad(struct _MyControls * This)
{
	MyCtrlStatus *controls = (MyCtrlStatus *)This->ctrls;
    if (controls->images)
        return;
    int i;
    char image_path[128] = {0};
    controls->images = (BITMAP *)calloc(controls->total_level,sizeof(BITMAP));
    for (i=0; i<controls->total_level; i++) {
        sprintf(image_path,"%s%s-%d.png",controls->relative_img_path,controls->img_name,i);
        bmpLoad(controls->images + i, image_path);
    }
}
static void myStatusBmpsRelease(struct _MyControls *This)
{
    int i;
	MyCtrlStatus *controls = (MyCtrlStatus *)This->ctrls;
    for (i=0; controls->idc != 0; i++) {
		bmpRelease(controls->images + i);
		free(controls->images + i);
		controls++;
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

MyControls * initMyStatus(void *controls)
{
	MyControls *This = (MyControls *)malloc(sizeof(MyControls));
	This->ctrls = controls;
	This->regist = myStatusRegist;
	This->unregist = myStatusCleanUp;
	This->bmpsLoad = myStatusBmpsLoad;
	This->bmpsRelease = myStatusBmpsRelease;

	return This;
}



