#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "commongdi.h"

//---------------------------------------------------------------------------
//绘制一个白色的底图
//---------------------------------------------------------------------------
void drawWhiteFrame (HWND hWnd, HDC hdc, const RECT* pClipRect,int Left,int Top,int Width,int Height)
{
//	BOOL fGetDC = FALSE;
//	RECT rcTemp;
//	RECT rcClient;

//	GetClientRect(hWnd,&rcClient);

//	if (hdc == 0) {
//		hdc = GetClientDC (hWnd);
//		fGetDC = TRUE;
//	}
//	if (pClipRect) {
//		rcTemp = *pClipRect;
//		ScreenToClient (hWnd, &rcTemp.left, &rcTemp.top);
//		ScreenToClient (hWnd, &rcTemp.right, &rcTemp.bottom);
//		IncludeClipRect(hdc,&rcTemp);
//	}
//	Draw3DControlFrame(hdc,Left,Top,Width,Height,COLOR_lightwhite,0);
//	if (fGetDC)
//		ReleaseDC (hdc);
}
//---------------------------------------------------------------------------
/* ---------------------------------------------------------------------------*/
/**
 * @brief drawBackground 绘制背景
 *
 * @param hWnd
 * @param hdc
 * @param pClipRect
 * @param Image
 */
/* ---------------------------------------------------------------------------*/
void drawBackground(HWND hWnd, HDC hdc, const RECT* pClipRect,BITMAP *Image)
{
    BOOL fGetDC = FALSE;
    RECT rcTemp;
	RECT rcClient;

	GetClientRect(hWnd,&rcClient);

    if (hdc == 0) {
        hdc = GetSecondaryClientDC (hWnd);
        fGetDC = TRUE;
    }
    if (pClipRect) {
        rcTemp = *pClipRect;
        ScreenToClient (hWnd, &rcTemp.left, &rcTemp.top);
        ScreenToClient (hWnd, &rcTemp.right, &rcTemp.bottom);
		IncludeClipRect(hdc,&rcTemp);
    }
	if (Image)
		FillBoxWithBitmap(hdc,rcClient.left,rcClient.top,RECTW(rcClient),RECTH(rcClient),Image);
	else {
		SetBrushColor (hdc,COLOR_black);
		FillBox (hdc, rcClient.left,rcClient.top,RECTW(rcClient),RECTH(rcClient));
	}
    if (fGetDC)
        ReleaseSecondaryDC (hWnd,hdc);
}
//----------------------------------------------------------------------------
void wndEraseBackground(HWND hWnd,HDC hdc, const RECT* pClipRect,BITMAP *pImage,int Left,int Top,int Width,int Height)
{
	BOOL fGetDC = FALSE;
	RECT rcTemp;

	if (hdc == 0) {
		hdc = GetClientDC (hWnd);
		fGetDC = TRUE;
	}
	if (pClipRect) {
		rcTemp = *pClipRect;
		ScreenToClient (hWnd, &rcTemp.left, &rcTemp.top);
		ScreenToClient (hWnd, &rcTemp.right, &rcTemp.bottom);
		IncludeClipRect(hdc,&rcTemp);
	}
	FillBoxWithBitmap(hdc,Left,Top,Width,Height,pImage);
	if (fGetDC)
		ReleaseDC (hdc);
}

/* ----------------------------------------------------------------*/
/**
 * @brief lineA2B 绘制直线
 *
 * @param hdc
 * @param a 起点
 * @param b 终点
 * @param color 颜色
 */
/* ----------------------------------------------------------------*/
void lineA2B (HDC hdc, POINT* a, POINT* b, int color)
{
	SetPenColor (hdc, color);
	MoveTo (hdc, a->x, a->y);
	LineTo (hdc, b->x, b->y);
}

/* ----------------------------------------------------------------*/
/**
 * @brief drawRectangle 绘制矩形 左上点(x0,y0) 右下点(x1,y1)
 *
 * @param hdc
 * @param x0
 * @param y0
 * @param x1
 * @param y1
 * @param color 颜色
 */
/* ----------------------------------------------------------------*/
void drawRectangle (HDC hdc,  int x0, int y0, int x1, int y1, int color)
{
	SetPenColor (hdc, color);
	MoveTo (hdc, x0, y0);
	LineTo (hdc, x1, y0);

	MoveTo (hdc, x1, y0);
	LineTo (hdc, x1,y1);

	MoveTo (hdc, x1,y1);
	LineTo (hdc, x0,y1);

	MoveTo (hdc, x0,y1);
	LineTo (hdc, x0, y0);
}

/* ----------------------------------------------------------------*/
/**
 * @brief drawTriangle
 * 填充三角形
 * @param hdc
 * @param rc
 * @param color
 * @param type 尖头方向 0:up 1:down 2:right 3:left
 */
/* ----------------------------------------------------------------*/
void drawTriangle (HDC hdc, RECT rc, int color, int type)
{
	SetPenColor (hdc, color);
	POINT* pts = (POINT*)malloc(sizeof(POINT) * 3);
    switch (type)
	{
		case 0:
			{
				pts->x = rc.left + RECTW(rc) / 2;
				pts->y = rc.top;

				(pts + 1)->x = rc.left;
				(pts + 1)->y = rc.bottom;

				(pts + 2)->x = rc.right;
				(pts + 2)->y = rc.bottom;
			} break;

		case 1:
			{
				pts->x = rc.left + RECTW(rc) / 2;
				pts->y = rc.bottom;

				(pts + 1)->x = rc.left;
				(pts + 1)->y = rc.top;

				(pts + 2)->x = rc.right;
				(pts + 2)->y = rc.top;

			} break;

		case 2:
			{
				pts->x = rc.right;
				pts->y = rc.top + RECTH(rc) / 2;

				(pts + 1)->x = rc.left;
				(pts + 1)->y = rc.top;

				(pts + 2)->x = rc.left;
				(pts + 2)->y = rc.bottom;

			} break;

		case 3:
			{
				pts->x = rc.left;
				pts->y = rc.top + RECTH(rc) / 2;

				(pts + 1)->x = rc.right;
				(pts + 1)->y = rc.top;

				(pts + 2)->x = rc.right;
				(pts + 2)->y = rc.bottom;

			} break;
	}
	FillPolygon(hdc,pts,3);
	free(pts);
}
/* ----------------------------------------------------------------*/
/**
 * @brief myFillBox 填充矩形区域颜色
 *
 * @param hdc
 * @param rc 矩形区域
 * @param color 颜色
 */
/* ----------------------------------------------------------------*/
void myFillBox(HDC hdc, RECT rc, int color)
{
	SetBrushColor (hdc,color);
	FillBox (hdc, rc.left, rc.top, RECTW(rc),RECTH(rc));
}
//----------------------------------------------------------------------------
//  显示一帧视频,VideoBuf为decode的缓冲区
//  缓冲区格式为DC_WIDTHXDC_HEIGHTX16Bit的RGB像素集
//----------------------------------------------------------------------------
void drawFrame(HWND hWnd,char *VideoBuf)
{
	// int y;
    // int width, height, pitch;				//绘制的宽度，高度和每行的字节数
    // RECT rc = {0, 0, 352, 288};				//DC_WIDTHXDC_HEIGHT
    // HDC hdc = GetDC(hWnd);		//在主界面直接绘制
    // int bpp = GetGDCapability (hdc, GDCAP_BPP);
    // Uint8* frame_buffer = LockDC (hdc, &rc, &width, &height, &pitch);	//帧缓冲区地址,锁定
    // Uint8* row = frame_buffer;				//行缓冲区地址
// //	VideoBuf += DC_WIDTH*(DC_HEIGHT-1)*DC_PIXEL;
    // for (y = 0; y < height; y++) {
        // FastCopy (row, VideoBuf, width * bpp);		//高速拷屏
        // row += pitch;								//下一行
		// VideoBuf += 352*2;				//decode缓冲区每行字节数为DC_HEIGHT*2=576Byte
    // }
    // UnlockDC (hdc);
    // ReleaseDC(hdc);
}
typedef struct BITMAPFILEHEADER
{
	unsigned short bfType;
	unsigned long  bfSize;
	unsigned short bfReserved1;
	unsigned short bfReserved2;
	unsigned long  bfOffBits;
} __attribute__ ((packed)) BITMAPFILEHEADER;

typedef struct BITMAPINFOHEADER  /* size: 40 */
{
	unsigned long  biSize;
	unsigned long  biWidth;
	unsigned long  biHeight;
	unsigned short biPlanes;
	unsigned short biBitCount;
	unsigned long  biCompression;
	unsigned long  biSizeImage;
	unsigned long  biXPelsPerMeter;
	unsigned long  biYPelsPerMeter;
	unsigned long  biClrUsed;
	unsigned long  biClrImportant;
} __attribute__ ((packed)) BITMAPINFOHEADER;


//---------------------------------------------------------------------------
void getPartFromBmp(const BITMAP *bmp,BITMAP *DstBitmap,int Left,int Top,int Width,int Height)
{
	int i,j;
	int DstLineCnt;
	int LineCnt;
	char *pBmpBuf;
	BITMAPFILEHEADER *head;
	BITMAPINFOHEADER *info;
	char *Pix;	//目标像素
	char *pSrc;	//源文件像素

	//目标每行缓冲区大小
	DstLineCnt = Width*3;
	if(DstLineCnt%4)
		DstLineCnt += 4-DstLineCnt%4;
	//分配空间
	pBmpBuf = (char*)malloc(sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+DstLineCnt*Height);
	head = (BITMAPFILEHEADER *)pBmpBuf;
	info = (BITMAPINFOHEADER *)&pBmpBuf[sizeof(BITMAPFILEHEADER)];

	head->bfType = 0x4d42;
	head->bfSize = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)+DstLineCnt*Height;
	head->bfOffBits = sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER);
	head->bfReserved1 = 0;
	head->bfReserved2 = 0;

	info->biSize = sizeof(BITMAPINFOHEADER);
	info->biWidth = Width;
	info->biHeight = Height;
	info->biPlanes = 1;
	info->biBitCount = 24;
	info->biCompression = 0;
	info->biSizeImage = 0;
	info->biXPelsPerMeter = 0;
	info->biYPelsPerMeter = 0;
	info->biClrUsed = 0;
	info->biClrImportant = 0;
	//一行数据多少
	LineCnt = bmp->bmWidth*2;
	if(LineCnt%4)
		LineCnt += 4-LineCnt%4;
	//首数据
	pSrc = &((char*)bmp->bmBits)[Top*LineCnt+Left*3];
	//最后一行
	Pix = &pBmpBuf[sizeof(BITMAPFILEHEADER)+sizeof(BITMAPINFOHEADER)]+(Height-1)*DstLineCnt;
	for(i=0;i<Height;i++) {
		char *pRGB = Pix;
		unsigned short *pRGB2 = (unsigned short *)pSrc;
		for(j=0;j<Width;j++) {
			Pixel2RGB(HDC_SCREEN,*pRGB2,&pRGB[2],&pRGB[1],&pRGB[0]);
			pRGB+=3;
			pRGB2++;
		}
		pSrc+=LineCnt;
		Pix-=DstLineCnt;
	}
	LoadBitmapFromMem(HDC_SCREEN,DstBitmap,pBmpBuf,head->bfSize,"png");
	free(pBmpBuf);
}

/* ----------------------------------------------------------------*/
/**
 * @brief setTranslate 设置透明按钮
 *
 * @param bmWidth
 * @param bmHeight
 * @param FgLine
 * @param BkLine
 * @param MkLine
 * @param FgPitch
 * @param BkPitch
 * @param MkPitch
 */
/* ----------------------------------------------------------------*/
static void setTranslate(DWORD bmWidth,DWORD bmHeight,char *FgLine,char *BkLine,char *MkLine,
		int FgPitch,int BkPitch,int MkPitch)
{
	DWORD i,j;
	WORD *BkBits,*FgBits,*MkBits;
	for(i = 0;i<bmHeight;i++) {
		FgBits = (WORD*)FgLine;
		BkBits = (WORD*)BkLine;
		MkBits = (WORD*)MkLine;
		for(j=0;j<bmWidth;j++) {
#if 0       //简单透明运算，速度快
			*FgBits = ((*BkBits)&(~(*MkBits))) | ((*FgBits) & (*MkBits));
#else       //支持透明度透明运算，速度较慢
			unsigned int dwTmp,A;
			A = (*MkBits>>5) & 0x3F;
			// R = (Bk * (64-A) + Fg * A) / 64;
			dwTmp = ((*BkBits & 0x1F)*(64-A) + (*FgBits & 0x1F)*A)>>6;      //R
			// R = ((Bk * (64-A) + Fg * A) / 64) << 5;
			dwTmp |= ((((*BkBits>>5) & 0x3F)*(64-A) + ((*FgBits>>5) & 0x3F)*A)>>1) & 0x7E0;     //G
			dwTmp |= ((((*BkBits>>11) & 0x1F)*(64-A) + ((*FgBits>>11) & 0x1F)*A)<<5) & 0xF800;      //B
			*BkBits = dwTmp;
#endif
			FgBits++;
			BkBits++;
			MkBits++;
		}
		FgLine += FgPitch;
		BkLine += BkPitch;
		MkLine += MkPitch;
	}
}

/* ----------------------------------------------------------------*/
/**
 * @brief translateIconPart 绘制透明图标
 *
 * @param hdc
 * @param x
 * @param y
 * @param w
 * @param h
 * @param FgBmp
 * @param LineIconCnt
 * @param IconIdx 绘制行中第几元素
 * @param t
 * @param Translate
 */
/* ----------------------------------------------------------------*/
void translateIconPart(HDC hdc,int x,int y,int w,int h,BITMAP *FgBmp,int LineIconCnt,int IconIdx,int t,
		BOOL Translate)
{
	if((LineIconCnt==4 || LineIconCnt==8) && Translate) {
		BITMAP BkBmp;
		char *BkLine,*FgLine,*MkLine;
		memset(&BkBmp,0,sizeof(BITMAP));
		GetBitmapFromDC(hdc,x,y,w,h,&BkBmp);
		FgLine = FgBmp->bmBits+t*FgBmp->bmPitch+w*IconIdx*2;
		BkLine = BkBmp.bmBits;
		if(LineIconCnt==4) {
			MkLine = FgLine+w*2*2;
		} else {
			MkLine = FgLine+w*4*2;
		}
		// setTranslate(w,h,FgLine,BkLine,MkLine,FgBmp->bmPitch,
				// BkBmp.bmPitch,FgBmp->bmPitch);
		FillBoxWithBitmap(hdc,x,y,w,h,&BkBmp);
		UnloadBitmap(&BkBmp);
	} else {
		if(LineIconCnt>IconIdx) {
			FillBoxWithBitmapPart (hdc, x, y, w, h, 0, 0, FgBmp,w*IconIdx, t);
		} else {
			FillBoxWithBitmapPart (hdc, x, y, w, h, 0, 0, FgBmp,w*(LineIconCnt-1), t);
		}
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief bmpsLoad 装载图片资源
 *
 * @param bmp 图片结构
 */
/* ---------------------------------------------------------------------------*/
void bmpLoad(BITMAP *bmp,char *path)
{
    char path_src[256];
	// char path_back[256];
    sprintf(path_src,"%s%s",BMP_RES_PATH,path);
    // sprintf(path_back,"%s%s",BMP_RESBACK_PATH,path);
    // printf("load back:%s\n",path_back );
    if (LoadBitmap (HDC_SCREEN,bmp, path_src)) {
        printf ("LoadBitmap(%s)fail.\n",path_src);
        // excuteCmd(1,"cp",path_back,path_src,NULL);
        // sync();
        // if (LoadBitmap (HDC_SCREEN,bmp, path_src))
            // printf ("LoadBitmaps(%s)fail-again!!.\n",path);
	} else {
        printf ("LoadBitmap(%s)ok.\n",path_src);
	}
}
void bmpsLoad(BmpLocation *bmp)
{
	int i;
	for (i=0; bmp->bmp != NULL; i++) {
		bmpLoad(bmp->bmp,bmp->location);
		bmp++;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief bmpsRelease 释放图片资源
 *
 * @param bmp 图片结构
 * @param num 图片数目
 */
/* ---------------------------------------------------------------------------*/
void bmpRelease(BITMAP *bmp)
{
    UnloadBitmap(bmp);
}

void bmpsRelease(BmpLocation *bmp)
{
	int i;
	for (i=0; bmp->bmp != NULL; i++) {
		bmpRelease(bmp[i].bmp);
	}
}
