/*
 * =====================================================================================
 *
 *       Filename:  FormVideoLayer.h
 *
 *    Description:  主窗口
 *
 *        Version:  1.0
 *        Created:  2016-02-23 15:34:38
 *       Revision:  1.0
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =====================================================================================
 */
#ifndef _FORM_VIDEOLAYER_H
#define _FORM_VIDEOLAYER_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include "commongdi.h"

	void formVideoLayerCreate(void);
    void formVideoLayerScreenOn(void);
	void formVideoLayerScreenOff(void);
	void formVideoLayerGotoPoweroff(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
