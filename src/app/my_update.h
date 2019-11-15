/*
 * =============================================================================
 *
 *       Filename:  my_update.h
 *
 *    Description:  升级流程
 *
 *        Version:  1.0
 *        Created:  2019-09-03 14:20:17 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _MY_UPDATE_H
#define _MY_UPDATE_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

	enum {
		UPDATE_FAIL,
		UPDATE_SUCCESS,
		UPDATE_POSITION
	};

	enum {
		UPDATE_TYPE_SERVER,
		UPDATE_TYPE_SDCARD,
		UPDATE_TYPE_CENTER,
	};

	typedef void (*UpdateFunc)(int result,int reason);
	typedef struct _MyUpdateInterface{
		void (*uiUpdateStart)(void);
		void (*uiUpdateDownloadSuccess)(void);
		void (*uiUpdateSuccess)(void);
		void (*uiUpdateFail)(void);
		void (*uiUpdatePos)(int pos);
		void (*uiUpdateStop)(void);
	}MyUpdateInterface;

	struct _MyUpdatePrivate;
	typedef struct _MyUpdate {
		struct _MyUpdatePrivate *priv;
		MyUpdateInterface *interface;
		int (*init)(struct _MyUpdate *,int type,char *ip,int port,char *file_path,UpdateFunc callbackFunc);
		int (*download)(struct _MyUpdate *);
		int (*uninit)(struct _MyUpdate *);
	}MyUpdate;

	void myUpdateInit(void);
	void myUpdateStart(int type,char *ip,int port,char *file_path);

	extern MyUpdate *my_update;
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
