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
#ifndef X86
#include <minigui/rkfb.h>
#endif

#include <stdint.h>

#include "form_idcs.h"

#define BMP_RES_PATH "res/image/"

#define SCR_WIDTH 1024           /*LCD Width     */
#define SCR_HEIGHT 600          /*LCD Height    */

    typedef struct _BmpLocation {
        BITMAP 	*bmp;
        char 	*location;
    }BmpLocation;

    typedef struct _FontLocation {
        PLOGFONT *font;
        int size;
        int first_type;
    }FontLocation;

#define STATIC_LB(x,y,w,h,id,caption,font,color)    \
	{"static",WS_CHILD|WS_VISIBLE|SS_CENTER,x,y,w,h,id,caption,0,WS_EX_TRANSPARENT,NULL,NULL,font,color}
#define STATIC_LB_LEFT(x,y,w,h,id,caption,font,color)    \
	{"static",WS_CHILD|WS_VISIBLE|SS_LEFT,x,y,w,h,id,caption,0,WS_EX_TRANSPARENT,NULL,NULL,font,color}
#define STATIC_IMAGE(x,y,w,h,id,dwAddData)  \
	{"static",WS_CHILD|WS_VISIBLE|SS_BITMAP|SS_CENTERIMAGE|SS_REALSIZEIMAGE,x,y,w,h,id,"",dwAddData,WS_EX_TRANSPARENT,NULL,NULL,NULL,0}
#define SCROLLVIEW(x,y,w,h,id)  \
	{"scrollview",WS_CHILD|WS_VISIBLE,x,y,w,h,id,"",0,WS_EX_TRANSPARENT,NULL,NULL,NULL,0}
#define ICONVIEW(x,y,w,h,id)  \
	{CTRL_ICONVIEW,WS_CHILD|WS_VISIBLE,x,y,w,h,id,"",0,WS_EX_NONE,NULL,NULL,NULL,0}
#define EDIT(x,y,w,h,id,caption,font,color)  \
	{CTRL_SLEDIT,WS_CHILD|WS_VISIBLE|ES_CENTER,\
	    x,y,w,h,id,caption,0,WS_EX_TRANSPARENT,NULL,NULL,font,color}


    void drawBackground(HWND hWnd, HDC hdc, const RECT* pClipRect,BITMAP *Image,int color);
	void drawWhiteFrame(HWND hWnd, HDC hdc, const RECT* pClipRect,int Left,int Top,int Width,int Height);
	void drawRectangle (HDC hdc,  int x0, int y0, int x1, int y1, int color);
	void wndEraseBackground(HWND hWnd,HDC hdc, const RECT* pClipRect,BITMAP *pImage,int Left,int Top,int Width,int Height);
	void getPartFromBmp(const BITMAP *bmp,BITMAP *DstBitmap,int Left,int Top,int Width,int Height);
	void myFillBox(HDC hdc, RECT *rc, int color);
	void translateIconPart(HDC hdc,int x,int y,int w,int h,BITMAP *FgBmp,int LineIconCnt,int IconIdx,int t,
			BOOL Translate);

    void bmpLoad(BITMAP *bmp,char *path);
    void bmpsLoad(BmpLocation *bmp);
    void bmpRelease(BITMAP *bmp);
    void bmpsRelease(BmpLocation *bmp);
    void fontsLoad(FontLocation *font);
    int myMoveWindow(HWND ctrl, int x,int y);

    extern PLOGFONT font36;
    extern PLOGFONT font22;
    extern PLOGFONT font20;
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
