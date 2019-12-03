/*
 * =====================================================================================
 *
 *       Filename:  MyBattery.c
 *
 *    Description:  自定义电池图标
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

#include "my_battery.h"
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
static int myBatteryControlProc (HWND hwnd, int message, WPARAM wParam, LPARAM lParam);

/* ----------------------------------------------------------------*
 *                        macro define
 *-----------------------------------------------------------------*/
#define CTRL_NAME CTRL_MYBATTERY
/* ----------------------------------------------------------------*
 *                      variables define
 *-----------------------------------------------------------------*/
MyControls *my_battery;
static BITMAP image_charging;// 关闭状态图片
static BITMAP image_normal;	// 打开状态图片

static BmpLocation base_bmps[] = {
	{&image_charging,"setting/ico_Battery_c.png"},
	{&image_normal, "setting/ico_Battery_nor.png"},
	{NULL},
};
/* ---------------------------------------------------------------------------*/
/**
 * @brief myBatteryRegist 注册控件
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static BOOL myBatteryRegist (void)
{
    WNDCLASS WndClass;

    WndClass.spClassName = CTRL_NAME;
    WndClass.dwStyle     = WS_NONE;
    WndClass.dwExStyle   = WS_EX_NONE;
    WndClass.hCursor     = GetSystemCursor (IDC_ARROW);
    WndClass.iBkColor    = 0;
    WndClass.WinProc     = myBatteryControlProc;

    return RegisterWindowClass(&WndClass);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myBatteryCleanUp 卸载控件
 */
/* ---------------------------------------------------------------------------*/
static void myBatteryCleanUp (void)
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

    char power_lever[16] = {0};
	RECT rc_bmp,rc_text;
    PCONTROL    pCtrl;
    pCtrl = Control (hWnd);
	GetClientRect (hWnd, &rc_text);

	if (!pCtrl->dwAddData2)
		return;
	MyBatteryCtrlInfo* pInfo = (MyBatteryCtrlInfo*)(pCtrl->dwAddData2);

    // 写标题
    SetTextColor(hdc,0xffffff);
	SetBkMode(hdc,BM_TRANSPARENT);
    SelectFont (hdc, pInfo->font);
	sprintf(power_lever,"%d%%",pInfo->ele_quantity);
    DrawText (hdc,power_lever, -1, &rc_text,
            DT_LEFT | DT_VCENTER | DT_WORDBREAK  | DT_SINGLELINE);
	SetBkMode(hdc,BM_OPAQUE);
	if (pInfo->state == 0) {
		// 正常状态
		rc_bmp.left = 64;
		rc_bmp.top = 13;
		rc_bmp.right = rc_bmp.left + pInfo->ele_quantity * 34 / 100 ;
		rc_bmp.bottom = rc_bmp.top + image_normal.bmHeight - 7;
		FillBoxWithBitmap(hdc,60,10,image_normal.bmWidth,image_normal.bmHeight,&image_normal);
		// 绘制电量
		if (pInfo->ele_quantity <= 10) {
			SetBrushColor (hdc, RGBA2Pixel (hdc, 0xFF, 0x00, 0x00, 0xFF));
		} else if (pInfo->ele_quantity <= 20) {
			SetBrushColor (hdc, RGBA2Pixel (hdc, 0xFF, 0x69, 0x00, 0xFF));
		} else {
			SetBrushColor (hdc, RGBA2Pixel (hdc, 0xFF, 0xFF, 0xFF, 0xFF));
		}
		FillBox (hdc, rc_bmp.left, rc_bmp.top, RECTW(rc_bmp),RECTH(rc_bmp));
		// myFillBox(hdc,&rc_bmp,0xffffff);
	} else {
		// 充电状态
		FillBoxWithBitmap(hdc,60,10,image_charging.bmWidth,image_charging.bmHeight,&image_charging);
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief myBatteryControlProc 控件主回调函数
 *
 * @param hwnd
 * @param message
 * @param wParam
 * @param lParam
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int myBatteryControlProc (HWND hwnd, int message, WPARAM wParam, LPARAM lParam)
{
    HDC         hdc;
    PCONTROL    pCtrl;
    DWORD       dwStyle;
    MyBatteryCtrlInfo* pInfo;

	pCtrl = Control (hwnd);
	pInfo = (MyBatteryCtrlInfo*)pCtrl->dwAddData2;
	dwStyle = GetWindowStyle (hwnd);

	switch (message) {
		case MSG_CREATE:
			{
				MyBatteryCtrlInfo* data = (MyBatteryCtrlInfo*)pCtrl->dwAddData;

				pInfo = (MyBatteryCtrlInfo*) calloc (1, sizeof (MyBatteryCtrlInfo));
				if (pInfo == NULL)
					return -1;
				memset(pInfo,0,sizeof(MyBatteryCtrlInfo));
				pInfo->font = data->font;
				pInfo->ele_quantity = data->ele_quantity;
				pInfo->state = data->state;
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

		case MSG_SET_QUANTITY:
			pInfo->ele_quantity = wParam;
            InvalidateRect (hwnd, NULL, TRUE);
			break;

		case MSG_SET_STATUS:
			pInfo->state = wParam;
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
static void myBatteryBmpsLoad(void *ctrls,char *path)
{
    bmpsLoad(base_bmps);
}
static void myBatteryBmpsRelease(void *ctrls)
{
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief createMyBattery 创建单个静态控件
 *
 * @param hWnd
 * @param id
 * @param x,y,w,h 坐标
 * @param image_normal 正常图片
 * @param image_press  按下图片
 * @param display 是否显示 0不显示 1显示
 * @param mode 是否为选择模式 0非选择 1选择
 */
/* ---------------------------------------------------------------------------*/
HWND createMyBattery(HWND hWnd,MyCtrlBattery *ctrl)
{
	HWND hCtrl;
	MyBatteryCtrlInfo pInfo;
	pInfo.font = ctrl->font;
	pInfo.ele_quantity = ctrl->ele_quantity;
	pInfo.state = ctrl->state;

    hCtrl = CreateWindowEx(CTRL_NAME,"",WS_VISIBLE|WS_CHILD,WS_EX_TRANSPARENT,
            ctrl->idc,ctrl->x,ctrl->y,ctrl->w,ctrl->h, hWnd,(DWORD)&pInfo);
    return hCtrl;
}

void initMyBattery(void)
{
	my_battery = (MyControls *)malloc(sizeof(MyControls));
	my_battery->regist = myBatteryRegist;
	my_battery->unregist = myBatteryCleanUp;
	my_battery->bmpsLoad = myBatteryBmpsLoad;
	my_battery->bmpsRelease = myBatteryBmpsRelease;
    my_battery->bmpsLoad(NULL,NULL);
}


