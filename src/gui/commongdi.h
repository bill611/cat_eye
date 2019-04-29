/*
 * =====================================================================================
 *
 *       Filename:  common.h
 *
 *    Description:  公共自定义画图函数
 *
 *        Version:  1.0
 *        Created:  2015-11-11 11:50:08
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =====================================================================================
 */
#ifndef _MY_COMMON_H
#define _MY_COMMON_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

#include <stdint.h>

#include "form_idcs.h"

#define BMP_RES_PATH "res/image/"

#define SCR_WIDTH 1024           /*LCD Width     */
#define SCR_HEIGHT 600          /*LCD Height    */

	enum {
		MSG_MYBUTTON_GET_SELECT_STATE = MSG_USER+1,
		MSG_MYBUTTON_SET_SELECT_MODE,
		MSG_MYBUTTON_SET_SELECT_STATE,
		MSG_MYBUTTON_SET_NORMAL_STATE,
		MSG_UPDATEMSG,
		MSG_SERIALPORT,
		MSG_UPDATESTATUS,
		MSG_SOCKETREAD,
	};

    typedef struct _BmpLocation {
        BITMAP 	*bmp;
        char 	*location;
    }BmpLocation;

    typedef struct _FontLocation {
        PLOGFONT *font;
        int size;
        int first_type;
    }FontLocation;

#define STATIC_LB(x,y,w,h,id,caption,dwAddData,font)    \
	{"static",WS_CHILD|WS_VISIBLE|SS_CENTER,x,y,w,h,id,caption,dwAddData,WS_EX_TRANSPARENT,NULL,NULL,font}
#define STATIC_IMAGE(x,y,w,h,id,dwAddData)  \
	    {"static",WS_CHILD|WS_VISIBLE|SS_BITMAP|SS_CENTERIMAGE,x,y,w,h,id,"",dwAddData,WS_EX_TRANSPARENT,NULL,NULL,NULL}


	void drawBackground(HWND hWnd, HDC hdc, const RECT* pClipRect,BITMAP *Image);
	void drawWhiteFrame(HWND hWnd, HDC hdc, const RECT* pClipRect,int Left,int Top,int Width,int Height);
	void wndEraseBackground(HWND hWnd,HDC hdc, const RECT* pClipRect,BITMAP *pImage,int Left,int Top,int Width,int Height);
	void getPartFromBmp(const BITMAP *bmp,BITMAP *DstBitmap,int Left,int Top,int Width,int Height);
	void myFillBox(HDC hdc, RECT rc, int color);
	void translateIconPart(HDC hdc,int x,int y,int w,int h,BITMAP *FgBmp,int LineIconCnt,int IconIdx,int t,
			BOOL Translate);

    void bmpLoad(BITMAP *bmp,char *path);
    void bmpsLoad(BmpLocation *bmp);
    void bmpRelease(BITMAP *bmp);
    void bmpsRelease(BmpLocation *bmp);
    void fontsLoad(FontLocation *font);

    extern PLOGFONT font22;
    extern PLOGFONT font20;
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
