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
static BITMAP image_swich_off;// 关闭状态图片
static BITMAP image_swich_on;	// 打开状态图片
static BITMAP image_add;	    // 添加按钮图片
static BITMAP image_exit;	    // 退出按钮图片

static BmpLocation base_bmps[] = {
	{&image_swich_off,"setting/Switch Off_小.png"},
	{&image_swich_on, "setting/Switch On_小.png"},
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
 * @brief clickInButton 判断触摸坐标是否在按钮内
 *
 * @param pInfo
 * @param x
 * @param y
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static int clickInButton(MyTitleCtrlInfo* pInfo,int x,int y)
{
	// 在原有图标返回上扩大触摸范围，提高手感
#define EXTEND_RC(hrc,offset) \
	do { \
		hrc.left -= offset;  \
		hrc.right += offset; \
		hrc.top -= offset; \
		hrc.bottom += offset; \
	} while (0)

    RECT rc ;
    int ret = MYTITLE_BUTTON_NULL;
    if (pInfo->flag_left == MYTITLE_LEFT_EXIT) {
		memcpy(&rc,&pInfo->bt_exit.rc,sizeof(RECT));
		EXTEND_RC(rc,10);
        if (PtInRect (&rc, x, y) && PtInRect (&rc, pInfo->click_x, pInfo->click_y))
            return MYTITLE_BUTTON_EXIT;
    }
    if (pInfo->flag_right == MYTITLE_RIGHT_ADD) {
		memcpy(&rc,&pInfo->bt_add.rc,sizeof(RECT));
		EXTEND_RC(rc,10);
        if (PtInRect (&rc, x, y) && PtInRect (&rc, pInfo->click_x, pInfo->click_y))
            ret = MYTITLE_BUTTON_ADD;
    } else if (pInfo->flag_right == MYTITLE_RIGHT_SWICH) {
		memcpy(&rc,&pInfo->bt_swich.rc,sizeof(RECT));
		EXTEND_RC(rc,10);
        if (PtInRect (&rc, x, y) && PtInRect (&rc, pInfo->click_x, pInfo->click_y)) {
            ret = MYTITLE_BUTTON_SWICH;
            if (pInfo->bt_swich.state == MYTITLE_SWICH_ON)
                pInfo->bt_swich.state = MYTITLE_SWICH_OFF;
            else
                pInfo->bt_swich.state = MYTITLE_SWICH_ON;
        }
	} else if (pInfo->flag_right == MYTITLE_RIGHT_TEXT) {
		memcpy(&rc,&pInfo->bt_add.rc,sizeof(RECT));
		EXTEND_RC(rc,10);
        if (PtInRect (&rc, x, y) && PtInRect (&rc, pInfo->click_x, pInfo->click_y))
            ret = MYTITLE_BUTTON_TEXT;
    }
    return ret;
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
	MyTitleCtrlInfo* pInfo = (MyTitleCtrlInfo*)(pCtrl->dwAddData2);

    // 绘制背景色
    myFillBox(hdc,rc_bmp,pInfo->bkg_color);
    // 写标题
    SetTextColor(hdc,pInfo->font_color);
    SetBkMode(hdc,BM_TRANSPARENT);
    SelectFont (hdc, pInfo->font);
    DrawText (hdc,pInfo->text, -1, &rc_text,
            DT_CENTER | DT_VCENTER | DT_WORDBREAK  | DT_SINGLELINE);
    if (pInfo->flag_left == MYTITLE_LEFT_EXIT) {
		FillBoxWithBitmap(hdc,FILL_BMP_STRUCT(pInfo->bt_exit.rc,pInfo->bt_exit.image_nor));
    }
    if (pInfo->flag_right == MYTITLE_RIGHT_TEXT) {
        rc_text.left = 930;
        DrawText (hdc,pInfo->text_right, -1, &rc_text,
                DT_CENTER | DT_VCENTER | DT_WORDBREAK  | DT_SINGLELINE);
    } else if (pInfo->flag_right == MYTITLE_RIGHT_ADD) {
		FillBoxWithBitmap(hdc,FILL_BMP_STRUCT(pInfo->bt_add.rc,pInfo->bt_add.image_nor));
    } else if (pInfo->flag_right == MYTITLE_RIGHT_SWICH) {
        if (pInfo->bt_swich.state == MYTITLE_SWICH_ON) {
            FillBoxWithBitmap(hdc,FILL_BMP_STRUCT(pInfo->bt_swich.rc,pInfo->bt_swich.image_pre));
        } else {
            FillBoxWithBitmap(hdc,FILL_BMP_STRUCT(pInfo->bt_swich.rc,pInfo->bt_swich.image_nor));
        }
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
                strcpy(pInfo->text_right,data->text_right);
				pInfo->font = data->font;
				pInfo->bkg_color= data->bkg_color;
				pInfo->font_color= data->font_color;

                // 退出按键设置
                pInfo->bt_exit.image_nor = &image_exit;
                SetRect(&pInfo->bt_exit.rc,16,10,
                        16 + pInfo->bt_exit.image_nor->bmWidth,
                        10 + pInfo->bt_exit.image_nor->bmHeight);
                // 添加按键设置
                pInfo->bt_add.image_nor = &image_add;
                SetRect(&pInfo->bt_add.rc,987,10,
                        987 + pInfo->bt_add.image_nor->bmWidth,
                        10 + pInfo->bt_add.image_nor->bmHeight);
                // 开关按键设置
                pInfo->bt_swich.image_nor = &image_swich_off;
                pInfo->bt_swich.image_pre = &image_swich_on;
                SetRect(&pInfo->bt_swich.rc,974,10,
                        974 + pInfo->bt_swich.image_nor->bmWidth,
                        10 + pInfo->bt_swich.image_nor->bmHeight);
                pInfo->bt_swich.state = MYTITLE_SWICH_OFF;

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

		case MSG_MYTITLE_SET_SWICH:
            pInfo->bt_swich.state = wParam;
            InvalidateRect (hwnd, NULL, TRUE);
			break;

        case MSG_LBUTTONDOWN:
            {
                int x, y;
                x = LOSWORD(lParam);
                y = HISWORD(lParam);
                if (GetCapture () == hwnd)
                    break;

                SetCapture (hwnd);
                pInfo->click_x = x;
                pInfo->click_y = y;
            }
            break;
        case MSG_LBUTTONUP:
            {
                int x, y;
                if (GetCapture() != hwnd) {
                    // if(pInfo->state!=BUT_NORMAL) {
                        // pInfo->state = BUT_NORMAL;
                        // InvalidateRect (hwnd, NULL, TRUE);
                    // }
                    break;
                }
                ReleaseCapture ();
                x = LOSWORD(lParam);
                y = HISWORD(lParam);
                int click_type = clickInButton(pInfo,x,y);
                if (click_type) {
                    NotifyParentEx (hwnd, pCtrl->id, click_type,pInfo->bt_swich.state);
                    InvalidateRect (hwnd, NULL, TRUE);
                }

            } break;

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
    if (ctrl->text_right)
        strcpy(pInfo.text_right,ctrl->text_right);
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


