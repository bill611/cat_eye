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
#if 0
#define FILL_BMP_STRUCT(rc,img)  rc.left, rc.top,img->bmWidth,img->bmHeight,img

	RECT rc_bmp,*rc_text;
    PCONTROL    pCtrl;
    pCtrl = Control (hWnd);
	GetClientRect (hWnd, &rc_bmp);
    rc_text = &rc_bmp;

	MyScrollCtrlInfo* pInfo = (MyScrollCtrlInfo*)(pCtrl->dwAddData2);
	if (!pCtrl->dwAddData2)
		return;

	if (pInfo->flag & MYBUTTON_TYPE_ONE_STATE) {
		SetTextColor(hdc,pInfo->color_nor);
		FillBoxWithBitmap(hdc,FILL_BMP_STRUCT(rc_bmp,pInfo->image_normal));
	} else if (pInfo->flag & MYBUTTON_TYPE_TWO_STATE) {
		if(pInfo->state == BUT_NORMAL) {
			SetTextColor(hdc,pInfo->color_nor);
			if (pInfo->flag & MYBUTTON_TYPE_PRESS_COLOR) {
				SetBrushColor (hdc,0x333333);
				FillBox (hdc, rc_bmp.left,rc_bmp.top,RECTW(rc_bmp),RECTH(rc_bmp));
			} else if (pInfo->flag & MYBUTTON_TYPE_PRESS_COLOR) {
			} else{
				FillBoxWithBitmap(hdc, FILL_BMP_STRUCT(rc_bmp,pInfo->image_normal));
			}
		} else {
			SetTextColor(hdc,pInfo->color_press);
			if (pInfo->flag & MYBUTTON_TYPE_PRESS_COLOR) {
				SetBrushColor (hdc,0x10B7F5);
				FillBox (hdc, rc_bmp.left,rc_bmp.top,RECTW(rc_bmp),RECTH(rc_bmp));
			} else if (pInfo->flag & MYBUTTON_TYPE_PRESS_COLOR) {
			} else{
				FillBoxWithBitmap(hdc, FILL_BMP_STRUCT(rc_bmp,pInfo->image_press));
			}
		}
	} else if (pInfo->flag & MYBUTTON_TYPE_CHECKBOX){
		if(pInfo->check == MYBUTTON_STATE_UNCHECK) {
			SetTextColor(hdc,pInfo->color_press);
			FillBoxWithBitmap(hdc, FILL_BMP_STRUCT(rc_bmp,pInfo->image_normal));
		} else {
			SetTextColor(hdc,pInfo->color_nor);
			FillBoxWithBitmap(hdc, FILL_BMP_STRUCT(rc_bmp,pInfo->image_press));
		}
	}
	if (!pInfo->text || (pInfo->flag & MYBUTTON_TYPE_TEXT_NULL))
		return;
	// SetTextColor(hdc,COLOR_lightwhite);
	SetBkMode(hdc,BM_TRANSPARENT);
	SelectFont (hdc, pInfo->font);
	if (pInfo->flag & MYBUTTON_TYPE_TEXT_CENTER) {
		if ((pInfo->flag & MYBUTTON_TYPE_PRESS_COLOR) == 0)
			rc_text->bottom = rc_bmp.top + pInfo->image_normal->bmHeight;
		DrawText (hdc,pInfo->text, -1, rc_text,
				DT_CENTER | DT_VCENTER | DT_WORDBREAK  | DT_SINGLELINE);
	} else {
		if ((pInfo->flag & MYBUTTON_TYPE_PRESS_COLOR) == 0)
			rc_text->top = rc_bmp.top + pInfo->image_normal->bmHeight;
		DrawText (hdc,pInfo->text, -1, rc_text,
				DT_CENTER | DT_BOTTOM| DT_VCENTER | DT_WORDBREAK  | DT_SINGLELINE);
	}
#endif
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
		pInfo->image_press = data->image_press;
		pInfo->image_normal = data->image_normal;
		pInfo->state = data->state;
		pInfo->flag = data->flag;
		pInfo->check = data->check;
		pInfo->text = data->text;
		pInfo->font = data->font;
		pInfo->color_nor = data->color_nor;
		pInfo->color_press = data->color_press;
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
    int i;
    char image_path[128] = {0};
	MyCtrlScroll *controls = (MyCtrlScroll *)ctrls;
    for (i=0; controls->idc != 0; i++) {
		sprintf(image_path,"%sico_%s_nor.png",path,controls->img_name);
		bmpLoad(&controls->image_normal, image_path);
		sprintf(image_path,"%sico_%s_pre.png",path,controls->img_name);
		bmpLoad(&controls->image_press, image_path);
		controls++;
    }
}
static void myScrollBmpsRelease(void *ctrls)
{
    int i;
	MyCtrlScroll *controls = (MyCtrlScroll *)ctrls;
    for (i=0; controls->idc != 0; i++) {
		bmpRelease(&controls->image_normal);
		bmpRelease(&controls->image_press);
		controls++;
    }
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
    int ctrl_w = 0,ctrl_h = 0;
	MyScrollCtrlInfo pInfo;
	pInfo.image_normal = &ctrl->image_normal;
	pInfo.image_press = &ctrl->image_press;
	pInfo.flag = ctrl->flag;
	pInfo.text = ctrl->img_name;
    pInfo.font = ctrl->font;
	pInfo.color_nor = COLOR_lightwhite;
	pInfo.color_press = COLOR_lightwhite;
	ctrl_w = ctrl->image_normal.bmWidth;
	ctrl_h = ctrl->image_normal.bmHeight;
	if (ctrl->font) {
		ctrl_h += pInfo.font->size + 30;
	}

    hCtrl = CreateWindowEx(CTRL_NAME,"",WS_VISIBLE|WS_CHILD,WS_EX_TRANSPARENT,
            ctrl->idc,ctrl->x,ctrl->y,ctrl_w,ctrl_h, hWnd,(DWORD)&pInfo);
	if(ctrl->notif_proc) {
		SetNotificationCallback (hCtrl, ctrl->notif_proc);
	}
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


