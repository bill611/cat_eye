/*
 * =====================================================================================
 *
 *       Filename:  MyStatic.c
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

#include "my_static.h"
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
static int myStaticControlProc (HWND hwnd, int message, WPARAM wParam, LPARAM lParam);

/* ----------------------------------------------------------------*
 *                        macro define
 *-----------------------------------------------------------------*/
#define CTRL_NAME CTRL_MYSTATIC
/* ----------------------------------------------------------------*
 *                      variables define
 *-----------------------------------------------------------------*/
MyControls *my_static;

/* ---------------------------------------------------------------------------*/
/**
 * @brief myStaticRegist 注册控件
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static BOOL myStaticRegist (void)
{
    WNDCLASS WndClass;

    WndClass.spClassName = CTRL_NAME;
    WndClass.dwStyle     = WS_NONE;
    WndClass.dwExStyle   = WS_EX_NONE;
    WndClass.hCursor     = GetSystemCursor (IDC_ARROW);
    WndClass.iBkColor    = 0;
    WndClass.WinProc     = myStaticControlProc;

    return RegisterWindowClass(&WndClass);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myStaticCleanUp 卸载控件
 */
/* ---------------------------------------------------------------------------*/
static void myStaticCleanUp (void)
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
#define FILL_BMP_STRUCT(rc,img)  rc.left, rc.top,img->bmWidth,img->bmHeight,img

	RECT rc_bmp,rc_text;
    PCONTROL    pCtrl;
    pCtrl = Control (hWnd);
	GetClientRect (hWnd, &rc_bmp);
    rc_text = rc_bmp;

	if (!pCtrl->dwAddData2)
		return;
	MyStaticCtrlInfo* pInfo = (MyStaticCtrlInfo*)(pCtrl->dwAddData2);

	if (pInfo->flag == MYSTATIC_TYPE_TEXT_AND_IMG)
		FillBoxWithBitmap(hdc,FILL_BMP_STRUCT(rc_bmp,pInfo->image));
	else
		myFillBox(hdc,rc_bmp,pInfo->bkg_color);

	if (pInfo->text) {
		SetTextColor(hdc,pInfo->font_color);
		SetBkMode(hdc,BM_TRANSPARENT);
		SelectFont (hdc, pInfo->font);
		DrawText (hdc,pInfo->text, -1, &rc_text,
				DT_CENTER | DT_VCENTER | DT_WORDBREAK  | DT_SINGLELINE);
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myStaticControlProc 控件主回调函数
 *
 * @param hwnd
 * @param message
 * @param wParam
 * @param lParam
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int myStaticControlProc (HWND hwnd, int message, WPARAM wParam, LPARAM lParam)
{
    HDC         hdc;
    PCONTROL    pCtrl;
    DWORD       dwStyle;
    MyStaticCtrlInfo* pInfo;

	pCtrl = Control (hwnd);
	pInfo = (MyStaticCtrlInfo*)pCtrl->dwAddData2;
	dwStyle = GetWindowStyle (hwnd);

	switch (message) {
		case MSG_CREATE:
			{
				MyStaticCtrlInfo* data = (MyStaticCtrlInfo*)pCtrl->dwAddData;

				pInfo = (MyStaticCtrlInfo*) calloc (1, sizeof (MyStaticCtrlInfo));
				if (pInfo == NULL)
					return -1;
				memset(pInfo,0,sizeof(MyStaticCtrlInfo));
				pInfo->image= data->image;
				pInfo->flag = data->flag;
				pInfo->text = data->text;
				pInfo->font = data->font;
				pInfo->bkg_color= data->bkg_color;
				pInfo->font_color= data->font_color;
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

		default:
			break;
	}

    return DefaultControlProc (hwnd, message, wParam, lParam);
}

/* ---------------------------------------------------------------------------*/
/**
  * @brief bmpsMainStaticLoad 加载主界面图片
  *
  * @param controls
**/
/* ---------------------------------------------------------------------------*/
static void myStaticBmpsLoad(void *ctrls,char *path)
{
    int i;
    char image_path[128] = {0};
	MyCtrlStatic *controls = (MyCtrlStatic *)ctrls;
    for (i=0; controls->idc != 0; i++) {
		if (controls->flag == MYSTATIC_TYPE_TEXT_AND_IMG) {
			sprintf(image_path,"%s%s.png",path,controls->img_name);
			bmpLoad(&controls->image, image_path);
		}
		controls++;
    }
}
static void myStaticBmpsRelease(void *ctrls)
{
    int i;
	MyCtrlStatic *controls = (MyCtrlStatic *)ctrls;
    for (i=0; controls->idc != 0; i++) {
		bmpRelease(&controls->image);
		controls++;
    }
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief createMyStatic 创建单个静态控件
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
HWND createMyStatic(HWND hWnd,MyCtrlStatic *ctrl)
{
	HWND hCtrl;
	MyStaticCtrlInfo pInfo;
	pInfo.image= &ctrl->image;
	pInfo.flag = ctrl->flag;
	pInfo.text = ctrl->text;
	pInfo.font = ctrl->font;
	pInfo.bkg_color= ctrl->bkg_color;
	pInfo.font_color= ctrl->font_color;

    hCtrl = CreateWindowEx(CTRL_NAME,"",WS_VISIBLE|WS_CHILD,WS_EX_TRANSPARENT,
            ctrl->idc,ctrl->x,ctrl->y,ctrl->w,ctrl->h, hWnd,(DWORD)&pInfo);
    return hCtrl;
}

void initMyStatic(void)
{
	my_static = (MyControls *)malloc(sizeof(MyControls));
	my_static->regist = myStaticRegist;
	my_static->unregist = myStaticCleanUp;
	my_static->bmpsLoad = myStaticBmpsLoad;
	my_static->bmpsRelease = myStaticBmpsRelease;
}


