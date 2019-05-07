/*
 * =====================================================================================
 *
 *       Filename:  MyTitle.c
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

#include "my_title.h"
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
static int myTitleControlProc (HWND hwnd, int message, WPARAM wParam, LPARAM lParam);

/* ----------------------------------------------------------------*
 *                        macro define
 *-----------------------------------------------------------------*/
#define CTRL_NAME CTRL_MYTITLE
/* ----------------------------------------------------------------*
 *                      variables define
 *-----------------------------------------------------------------*/
MyControls *my_title;
static BITMAP image_swich_close;// 关闭状态图片
static BITMAP image_swich_open;	// 打开状态图片
static BITMAP image_add;	    // 添加按钮图片
static BITMAP image_exit;	    // 退出按钮图片

static BmpLocation base_bmps[] = {
	{&image_swich_close,"setting/Switch Off_小.png"},
	{&image_swich_open, "setting/Switch On_小.png"},
	{&image_add,        "setting/ico_添加.png"},
	{&image_exit,       "setting/arrow-back.png"},
	{NULL},
};
/* ---------------------------------------------------------------------------*/
/**
 * @brief myTitleRegist 注册控件
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static BOOL myTitleRegist (void)
{
    WNDCLASS WndClass;

    WndClass.spClassName = CTRL_NAME;
    WndClass.dwStyle     = WS_NONE;
    WndClass.dwExStyle   = WS_EX_NONE;
    WndClass.hCursor     = GetSystemCursor (IDC_ARROW);
    WndClass.iBkColor    = 0;
    WndClass.WinProc     = myTitleControlProc;

    return RegisterWindowClass(&WndClass);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myTitleCleanUp 卸载控件
 */
/* ---------------------------------------------------------------------------*/
static void myTitleCleanUp (void)
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
#define FILL_BMP_STRUCT(rc,img)  rc.left, rc.top,img.bmWidth,img.bmHeight,&img

	RECT rc_bmp,rc_text;
    PCONTROL    pCtrl;
    pCtrl = Control (hWnd);
	GetClientRect (hWnd, &rc_bmp);
    rc_text = rc_bmp;

	if (!pCtrl->dwAddData2)
		return;
	MyTitleCtrlInfo* pInfo = (MyTitleCtrlInfo*)(pCtrl->dwAddData2);

    // 绘制背景色
    myFillBox(hdc,rc_bmp,pInfo->bkg_color);
    // 写标题
    SetTextColor(hdc,pInfo->font_color);
    SetBkMode(hdc,BM_TRANSPARENT);
    SelectFont (hdc, pInfo->font);
    DrawText (hdc,pInfo->text, -1, &rc_text,
            DT_CENTER | DT_VCENTER | DT_WORDBREAK  | DT_SINGLELINE);
    if (pInfo->flag_right == MYTITLE_LEFT_EXIT) {
		FillBoxWithBitmap(hdc,FILL_BMP_STRUCT(rc_bmp,image_exit));
    }
    if (pInfo->flag_right == MYTITLE_RIGHT_TEXT) {
        rc_text.left = 930; 
        DrawText (hdc,pInfo->text_right, -1, &rc_text,
                DT_CENTER | DT_VCENTER | DT_WORDBREAK  | DT_SINGLELINE);
    } else if (pInfo->flag_right == MYTITLE_RIGHT_ADD) {
        rc_bmp.left = 987;
		FillBoxWithBitmap(hdc,FILL_BMP_STRUCT(rc_bmp,image_add));
    }
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myTitleControlProc 控件主回调函数
 *
 * @param hwnd
 * @param message
 * @param wParam
 * @param lParam
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int myTitleControlProc (HWND hwnd, int message, WPARAM wParam, LPARAM lParam)
{
    HDC         hdc;
    PCONTROL    pCtrl;
    DWORD       dwStyle;
    MyTitleCtrlInfo* pInfo;

	pCtrl = Control (hwnd);
	pInfo = (MyTitleCtrlInfo*)pCtrl->dwAddData2;
	dwStyle = GetWindowStyle (hwnd);

	switch (message) {
		case MSG_CREATE:
			{
				MyTitleCtrlInfo* data = (MyTitleCtrlInfo*)pCtrl->dwAddData;

				pInfo = (MyTitleCtrlInfo*) calloc (1, sizeof (MyTitleCtrlInfo));
				if (pInfo == NULL)
					return -1;
				memset(pInfo,0,sizeof(MyTitleCtrlInfo));
				pInfo->flag_left = data->flag_left;
				pInfo->flag_right = data->flag_right;
                strcpy(pInfo->text,data->text);
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

		case MSG_MYTITLE_SET_TITLE:
            if (wParam)
                strcpy(pInfo->text,(char*)wParam);
            InvalidateRect (hwnd, NULL, TRUE);
			break;

		default:
			break;
	}

    return DefaultControlProc (hwnd, message, wParam, lParam);
}

/* ---------------------------------------------------------------------------*/
/**
  * @brief bmpsMainTitleLoad 加载主界面图片
  *
  * @param controls
**/
/* ---------------------------------------------------------------------------*/
static void myTitleBmpsLoad(void *ctrls,char *path)
{
    bmpsLoad(base_bmps);
}
static void myTitleBmpsRelease(void *ctrls)
{
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief createMyTitle 创建单个静态控件
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
HWND createMyTitle(HWND hWnd,MyCtrlTitle *ctrl)
{
	HWND hCtrl;
	MyTitleCtrlInfo pInfo;
    pInfo.flag_left = ctrl->flag_left;
    pInfo.flag_right = ctrl->flag_right;
    if (ctrl->text)
        strcpy(pInfo.text,ctrl->text);
	pInfo.font = ctrl->font;
	pInfo.bkg_color= ctrl->bkg_color;
	pInfo.font_color= ctrl->font_color;

    hCtrl = CreateWindowEx(CTRL_NAME,"",WS_VISIBLE|WS_CHILD,WS_EX_TRANSPARENT,
            ctrl->idc,ctrl->x,ctrl->y,ctrl->w,ctrl->h, hWnd,(DWORD)&pInfo);
	if(ctrl->notif_proc) {
		SetNotificationCallback (hCtrl, ctrl->notif_proc);
	}
    return hCtrl;
}

void initMyTitle(void)
{
	my_title = (MyControls *)malloc(sizeof(MyControls));
	my_title->regist = myTitleRegist;
	my_title->unregist = myTitleCleanUp;
	my_title->bmpsLoad = myTitleBmpsLoad;
	my_title->bmpsRelease = myTitleBmpsRelease;
    my_title->bmpsLoad(NULL,NULL);
}


