/*
 * =====================================================================================
 *
 *       Filename:  MyButton.c
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

#include "my_button.h"
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
#define CTRL_NAME CTRL_MYBUTTON
/* ----------------------------------------------------------------*
 *                      variables define
 *-----------------------------------------------------------------*/
MyControls *my_button;

/* ---------------------------------------------------------------------------*/
/**
 * @brief myButtonRegist 注册控件
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static BOOL myButtonRegist (void)
{
    WNDCLASS WndClass;

    WndClass.spClassName = CTRL_NAME;
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
static void myButtonCleanUp (void)
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
	RECT rc_bmp,rc_text;
    PCONTROL    pCtrl;
    pCtrl = Control (hWnd);
	GetClientRect (hWnd, &rc_bmp);
    rc_text = rc_bmp;

	if (pCtrl->dwAddData2) {
		MyButtonCtrlInfo* pInfo = (MyButtonCtrlInfo*)(pCtrl->dwAddData2);

		if (pInfo->select.mode == 1) {
			if(pInfo->select.state == BUT_STATE_SELECT)
                FillBoxWithBitmap(hdc,
                        rc_bmp.left,
                        rc_bmp.top,
                        pInfo->image_press->bmWidth,
                        pInfo->image_press->bmHeight,
                        pInfo->image_press);
            else
                FillBoxWithBitmap(hdc,
                        rc_bmp.left,
                        rc_bmp.top,
                        pInfo->image_normal->bmWidth,
                        pInfo->image_normal->bmHeight,
                        pInfo->image_normal);
		} else {
            if(pInfo->state == BUT_NORMAL)
                FillBoxWithBitmap(hdc,
                        rc_bmp.left,
                        rc_bmp.top,
                        pInfo->image_normal->bmWidth,
                        pInfo->image_normal->bmHeight,
                        pInfo->image_normal);
            else
                FillBoxWithBitmap(hdc,
                        rc_bmp.left,
                        rc_bmp.top,
                        pInfo->image_press->bmWidth,
                        pInfo->image_press->bmHeight,
                        pInfo->image_press);

		}
        rc_text.top = rc_bmp.top + pInfo->image_normal->bmHeight;
        if (pInfo->text) {
            SetTextColor(hdc,COLOR_lightwhite);
            SetBkMode(hdc,BM_TRANSPARENT);
            SelectFont (hdc, pInfo->font);
            DrawText (hdc,pInfo->text, -1, &rc_text,
                    DT_CENTER | DT_BOTTOM| DT_VCENTER | DT_WORDBREAK  | DT_SINGLELINE);
        }
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
    MyButtonCtrlInfo* pInfo;

	pCtrl = Control (hwnd);
	pInfo = (MyButtonCtrlInfo*)pCtrl->dwAddData2;
	dwStyle = GetWindowStyle (hwnd);

	switch (message) {
	case MSG_CREATE:
	{
		MyButtonCtrlInfo* data = (MyButtonCtrlInfo*)pCtrl->dwAddData;

		pInfo = (MyButtonCtrlInfo*) calloc (1, sizeof (MyButtonCtrlInfo));
		if (pInfo == NULL)
			return -1;
		memset(pInfo,0,sizeof(MyButtonCtrlInfo));
		pInfo->image_press = data->image_press;
		pInfo->image_normal = data->image_normal;
		pInfo->state = data->state;
		pInfo->select.mode = data->select.mode;
		pInfo->select.state = data->select.state;
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

    case MSG_MYBUTTON_GET_SELECT_STATE:
		return pInfo->select.state;

    case MSG_MYBUTTON_SET_SELECT_STATE:
		if ((int)wParam)
			pInfo->select.state = BUT_STATE_SELECT;
		else
			pInfo->select.state = BUT_STATE_UNSELECT;
		InvalidateRect (hwnd, NULL, FALSE);
		return 0;

    case MSG_MYBUTTON_SET_NORMAL_STATE:
		if ((int)wParam)
			pInfo->state = BUT_CLICK;
		else
			pInfo->state = BUT_NORMAL;
		InvalidateRect (hwnd, NULL, FALSE);
		return 0;

    case MSG_MYBUTTON_SET_SELECT_MODE:
		pInfo->select.mode = (int)wParam;
        if (pInfo->select.mode != 3)
            InvalidateRect (hwnd, NULL, FALSE);
		return 0;

	case MSG_ENABLE:
		if (wParam && (dwStyle & WS_DISABLED)) {
			pCtrl->dwStyle &= ~WS_DISABLED;
			pInfo->state = BUT_NORMAL;
		}
		else if (!wParam && !(dwStyle & WS_DISABLED)) {
			pCtrl->dwStyle |= WS_DISABLED;
			pInfo->state = BUT_DISABLED;
		}
		else
			return 0;
		InvalidateRect (hwnd, NULL, FALSE);
		return 0;

    case MSG_LBUTTONDOWN:
        if (GetCapture () == hwnd)
            break;

        if(pInfo->state == BUT_CLICK)
			break;			//已经是选中状态

        SetCapture (hwnd);
		pInfo->state = BUT_CLICK;
		NotifyParent (hwnd, pCtrl->id, BN_PUSHED);
		InvalidateRect (hwnd, NULL, TRUE);
        break;
	case MSG_LBUTTONUP:
	{
        int x, y;
		if (GetCapture() != hwnd) {
			if(!(dwStyle & BS_CHECKBOX)) {
				if(pInfo->state!=BUT_NORMAL) {
					pInfo->state = BUT_NORMAL;
                    if (pInfo->select.mode != 3)
                        InvalidateRect (hwnd, NULL, TRUE);
				}
			}
            break;
		}

        ReleaseCapture ();
		if(!(dwStyle & BS_CHECKBOX)) {
			pInfo->state = BUT_NORMAL;
#ifdef X86
			if (pInfo->select.mode){
				if (pInfo->select.state == BUT_STATE_SELECT)
					pInfo->select.state = BUT_STATE_UNSELECT;
				else
					pInfo->select.state = BUT_STATE_SELECT;
			}
			NotifyParent (hwnd, pCtrl->id, BN_CLICKED);
#endif
        if (pInfo->select.mode != 3)
			InvalidateRect (hwnd, NULL, TRUE);
		}
		NotifyParent (hwnd, pCtrl->id, BN_UNPUSHED);
		// InvalidateRect (hwnd, NULL, TRUE);

		x = LOSWORD(lParam);
        y = HISWORD(lParam);
		//设置捕获后，坐标变成屏幕绝对值，需要转换为窗口坐标
		if(wParam & KS_CAPTURED) {
			ScreenToClient (GetParent (hwnd), &x, &y);
		}

		if (PtInRect ((PRECT) &(pCtrl->cl), x, y))
        {
			// playButtonSound();
			if (pInfo->select.mode){
				if (pInfo->select.state == BUT_STATE_SELECT)
					pInfo->select.state = BUT_STATE_UNSELECT;
				else
					pInfo->select.state = BUT_STATE_SELECT;
			}
			NotifyParent (hwnd, pCtrl->id, BN_CLICKED);
            if (pInfo->select.mode != 3)
                InvalidateRect (hwnd, NULL, TRUE);
		} else if(dwStyle & BS_CHECKBOX) {
			pInfo->state = BUT_NORMAL;
            if (pInfo->select.mode != 3)
                InvalidateRect (hwnd, NULL, TRUE);
		}
		break;
	}
	default:
		break;
    }

    return DefaultControlProc (hwnd, message, wParam, lParam);
}

/* ---------------------------------------------------------------------------*/
/**
  * @brief bmpsMainButtonLoad 加载主界面图片
  *
  * @param controls
**/
/* ---------------------------------------------------------------------------*/
static void myButtonBmpsLoad(void *ctrls,char *path)
{
    int i;
    char image_path[128] = {0};
	MyCtrlButton *controls = (MyCtrlButton *)ctrls;
    for (i=0; controls->idc != 0; i++) {
        sprintf(image_path,"%sico_%s_nor.png",path,controls->img_name);
        bmpLoad(&controls->image_normal, image_path);
        sprintf(image_path,"%sico_%s_pre.png",path,controls->img_name);
        bmpLoad(&controls->image_press, image_path);
		controls++;
    }
}
static void myButtonBmpsRelease(void *ctrls)
{
    int i;
	MyCtrlButton *controls = (MyCtrlButton *)ctrls;
    for (i=0; controls->idc != 0; i++) {
		bmpRelease(&controls->image_normal);
		bmpRelease(&controls->image_press);
		controls++;
    }
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief createMyButton 创建单个皮肤按钮
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
HWND createMyButton(HWND hWnd,MyCtrlButton *ctrl,const char *text, int mode)
{
	HWND hCtrl;
    int ctrl_w,ctrl_h = 0;
	MyButtonCtrlInfo pInfo;
	pInfo.image_normal = &ctrl->image_normal;
	pInfo.image_press = &ctrl->image_press;
	pInfo.select.mode = mode;
	pInfo.state = BUT_NORMAL;
	pInfo.select.state = BUT_STATE_SELECT;
	pInfo.text = text;
    pInfo.font = ctrl->font;
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

void initMyButton(void)
{
	my_button = (MyControls *)malloc(sizeof(MyControls));
	my_button->regist = myButtonRegist;
	my_button->unregist = myButtonCleanUp;
	my_button->bmpsLoad = myButtonBmpsLoad;
	my_button->bmpsRelease = myButtonBmpsRelease;
}


