#include <stdio.h>
#include <stdlib.h>
#include <memory.h>


#include "commongdi.h"
#include "cliprect.h"
#include "internals.h"
#include "ctrlclass.h"

#include "my_dialog.h"



HWND GUIAPI CreateMyWindowIndirectParamEx (PMY_DLGTEMPLATE pDlgTemplate,
		HWND hOwner, WNDPROC WndProc, LPARAM lParam,
		const char* werdr_name, WINDOW_ELEMENT_ATTR* we_attrs,
		const char* window_name, const char* layer_name)
{
	MAINWINCREATE CreateInfo;
	HWND hMainWin;
	int i;
	PMY_CTRLDATA pCtrlData;
	HWND hCtrl;
	HWND hFocus;
	if (pDlgTemplate->controlnr > 0 && !pDlgTemplate->controls)
		return HWND_INVALID;
	hOwner = GetMainWindowHandle (hOwner);

	CreateInfo.dwReserved     = (DWORD)pDlgTemplate;

	CreateInfo.dwStyle        = pDlgTemplate->dwStyle & ~WS_VISIBLE;
	CreateInfo.dwExStyle      = pDlgTemplate->dwExStyle ;//| WS_EX_AUTOSECONDARYDC;
	CreateInfo.spCaption      = pDlgTemplate->caption;
	CreateInfo.hMenu          = pDlgTemplate->hMenu;
	CreateInfo.hCursor        = GetSystemCursor (IDC_ARROW);
	CreateInfo.hIcon          = pDlgTemplate->hIcon;
	CreateInfo.MainWindowProc = WndProc ? WndProc : DefaultMainWinProc;
	CreateInfo.lx             = pDlgTemplate->x;
	CreateInfo.ty             = pDlgTemplate->y;
	CreateInfo.rx             = pDlgTemplate->x + pDlgTemplate->w;
	CreateInfo.by             = pDlgTemplate->y + pDlgTemplate->h;
	CreateInfo.iBkColor       =
		0;
	CreateInfo.dwAddData      = pDlgTemplate->dwAddData;
	CreateInfo.hHosting       = hOwner;


	hMainWin = CreateMainWindowEx (&CreateInfo,
			werdr_name, we_attrs, window_name, layer_name);

	if (hMainWin == HWND_INVALID)
		return HWND_INVALID;


	for (i = 0; i < pDlgTemplate->controlnr; i++) {

		pCtrlData = pDlgTemplate->controls + i;
		hCtrl = CreateWindowEx2 (pCtrlData->class_name,
				pCtrlData->caption,
				pCtrlData->dwStyle | WS_CHILD,
				pCtrlData->dwExStyle,
				pCtrlData->id,
				pCtrlData->x,
				pCtrlData->y,
				pCtrlData->w,
				pCtrlData->h,
				hMainWin,
				pCtrlData->werdr_name,
				pCtrlData->we_attrs,
				pCtrlData->dwAddData);

		if (hCtrl == HWND_INVALID) {
			DestroyMainWindow (hMainWin);
			MainWindowThreadCleanup (hMainWin);
			return HWND_INVALID;
		}
		if (pCtrlData->font != NULL)
	       SetWindowFont(hCtrl,*pCtrlData->font);
	}

#if 0
	hFocus = GetNextDlgTabItem (hMainWin, (HWND)0, FALSE);
#else
	/* houhh 20100706, set the forefront control as focus. */
	{
		PCONTROL pCtrl;
		for (pCtrl = (PCONTROL)(((PMAINWIN)hMainWin)->hFirstChild);
				pCtrl && pCtrl->next; pCtrl = pCtrl->next);
		hFocus = (HWND)pCtrl;

	}
#endif

	if (SendMessage (hMainWin, MSG_INITDIALOG, hFocus, lParam)) {
		if (hFocus)
			SetFocus (hFocus);
	}

	if (!(pDlgTemplate->dwExStyle & WS_EX_DLGHIDE)) {
		ShowWindow (hMainWin, SW_SHOWNORMAL);
	}

	return hMainWin;
}


