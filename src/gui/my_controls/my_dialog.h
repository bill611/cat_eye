
#ifndef _MGUI_CTRL_MYDIALOG_H
#define _MGUI_CTRL_MYDIALOG_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <minigui/common.h>
#include <minigui/minigui.h>
#include <minigui/gdi.h>
#include <minigui/window.h>
#include <minigui/control.h>

typedef struct {
//	NOTIFPROC notif_proc;
 /** Class name of the control */
    const char* class_name;
    /** Control style */
    DWORD       dwStyle;
    /** Control position in dialog */
    int         x, y, w, h;
    /** Control identifier */
    int         id;
    /** Control caption */
    const char* caption;
    /** Additional data */
    DWORD       dwAddData;
    /** Control extended style */
    DWORD       dwExStyle;

    /** window element renderer name */
    const char* werdr_name;

    /** table of we_attrs */
    const WINDOW_ELEMENT_ATTR* we_attrs;

	PLOGFONT *font;
    int font_color;
}MY_CTRLDATA;

typedef MY_CTRLDATA* PMY_CTRLDATA;


typedef struct {
    /** Dialog box style */
    DWORD       dwStyle;
    /** Dialog box extended style */
    DWORD       dwExStyle;
    /** Dialog box position */
    int         x, y, w, h;
    /** Dialog box caption */
    const char* caption;
    /** Dialog box icon */
    HICON       hIcon;
    /** Dialog box menu */
    HMENU       hMenu;
    /** Number of controls */
    int         controlnr;
    /** Poiter to control array */
    PMY_CTRLDATA   controls;
    /** Addtional data, must be zero */
    DWORD       dwAddData;

}MY_DLGTEMPLATE;

typedef MY_DLGTEMPLATE* PMY_DLGTEMPLATE;


HWND GUIAPI CreateMyWindowIndirectParamEx (PMY_DLGTEMPLATE pDlgTemplate,
        HWND hOwner, WNDPROC WndProc, LPARAM lParam,
        const char* werdr_name, WINDOW_ELEMENT_ATTR* we_attrs,
        const char* window_name, const char* layer_name);


static inline HWND GUIAPI CreateMyWindowIndirectParam (
                PMY_DLGTEMPLATE pDlgTemplate, HWND hOwner,
                WNDPROC WndProc, LPARAM lParam)
{
    return CreateMyWindowIndirectParamEx (pDlgTemplate, hOwner,
            WndProc, lParam, NULL, NULL, NULL, NULL);
}


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif /* _MGUI_CTRL_ICONVIEW_H */
