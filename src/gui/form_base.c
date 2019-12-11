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
				this->priv->auto_close_time = this->priv->auto_close_time_set;
				if (this->priv->initPara)
					this->priv->initPara(hDlg,message,wParam,lParam);
                SetTimer(hDlg,this->priv->idc_timer,FORM_TIMER_1S);

			} return FORM_CONTINUE;

		case MSG_MOUSEMOVE:
		case MSG_LBUTTONDOWN:
			{
				this->priv->auto_close_time = this->priv->auto_close_time_set;
				// screensaverStart(LCD_ON);
			} return FORM_CONTINUE;

		case MSG_TIMER:
			{
				if (wParam != (WPARAM)this->priv->idc_timer){
					return FORM_CONTINUE;
				}
				if (this->priv->auto_close_time > 0) {
					// printf("[%s]auto close:%d\n", this->priv->name,this->priv->auto_close_time);
					if (--this->priv->auto_close_time == 0) {
						// SendMessage(hDlg, MSG_CLOSE,0,0);
						ShowWindow(hDlg,SW_HIDE);
					}
				}
			} return FORM_CONTINUE;

		case MSG_SHOWWINDOW:
			{
				if (wParam == SW_HIDE) {
					this->priv->auto_close_time = 0;
					if(this->priv->callBack)
						this->priv->callBack();
				} else if (wParam == SW_SHOWNORMAL) {
					this->priv->auto_close_time = this->priv->auto_close_time_set;
				}
			}return FORM_CONTINUE;
		case MSG_ERASEBKGND:
			{
				drawBackground(hDlg,
						(HDC)wParam,
						(const RECT*)lParam, this->priv->bmp_bkg,0x0);
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
	if (priv->auto_close_time_set == 0)
		this->priv->auto_close_time_set = FORM_SETTING_ONTIME;
	return this;
}

