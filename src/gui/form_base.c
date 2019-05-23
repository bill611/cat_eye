/*
 * =============================================================================
 *
 *       Filename:  FormBase.c
 *
 *    Description:  设置类基本窗口框架
 *
 *        Version:  1.0
 *        Created:  2016-02-19 15:22:58
 *       Revision:  1.0
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
/* ---------------------------------------------------------------------------*
 *                      include head files
 *----------------------------------------------------------------------------*/

#include "screen.h"
#include "externfunc.h"
#include "form_base.h"
/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#if DBG_FORM_BASE > 0
	#define DBG_P( x... ) printf( x )
#else
	#define DBG_P( x... )
#endif

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/

static int formBaseProc(FormBase *this,HWND hDlg, int message, WPARAM wParam, LPARAM lParam)
{
	switch (message)
	{
		case MSG_INITDIALOG:
			{
				Screen.Add(hDlg,this->priv->name);
				this->hDlg = hDlg;
				this->auto_close_time = FORM_SETTING_ONTIME;
				if (this->priv->initPara)
					this->priv->initPara(hDlg,message,wParam,lParam);
                SetTimer(hDlg,this->priv->idc_timer,100);

			} return FORM_CONTINUE;

		case MSG_MOUSEMOVE:
		case MSG_LBUTTONDOWN:
			{
				this->auto_close_time = FORM_SETTING_ONTIME;
				// screensaverStart(LCD_ON);
			} return FORM_CONTINUE;

		case MSG_TIMER:
			{
				if (wParam != this->priv->idc_timer){
					return FORM_CONTINUE;
				}
				if (this->auto_close_time > 0) {
					// printf("[%s]auto close:%d\n", this->priv->name,this->auto_close_time);
					if (--this->auto_close_time == 0) {
                        // SendMessage(hDlg, MSG_CLOSE,0,0);
						ShowWindow(hDlg,SW_HIDE);
					}
				}
			} return FORM_CONTINUE;

		case MSG_SHOWWINDOW:
			{
				if (wParam == SW_HIDE) {
					this->auto_close_time = 0;
					if(this->priv->callBack)
						this->priv->callBack();
				} else if (wParam == SW_SHOWNORMAL) {
					this->auto_close_time = FORM_SETTING_ONTIME;
				}
			}return FORM_CONTINUE;
		case MSG_ERASEBKGND:
			{
				drawBackground(hDlg,
						(HDC)wParam,
						(const RECT*)lParam, this->priv->bmp_bkg,0);
			} return FORM_STOP;

		case MSG_CLOSE:
			{
				if (IsTimerInstalled(hDlg,this->priv->idc_timer) == TRUE)
                    KillTimer (hDlg,this->priv->idc_timer);
				DestroyMainWindow (hDlg);
				MainWindowThreadCleanup (hDlg);
			} return FORM_STOP;

		case MSG_DESTROY:
			{
				Screen.Del(hDlg);
				DestroyAllControls(hDlg);
			} return FORM_STOP;

		default:
			return FORM_CONTINUE;
	}
}

FormBase * formBaseCreate(FormBasePriv *priv)
{
	FormBase *this = (FormBase *) calloc (1,sizeof(FormBase));
	this->priv = (FormBasePriv *) calloc (1,sizeof(FormBasePriv));

	this->priv = priv;
	this->baseProc = formBaseProc;
	return this;
}

