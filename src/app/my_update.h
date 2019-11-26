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
		UPDATE_FAIL,	 // 升级失败
		UPDATE_SUCCESS,  // 升级成功
		UPDATE_POSITION  // 升级百分比
	};

	enum {
		UPDATE_TYPE_NONE,   // 无需升级
		UPDATE_TYPE_SERVER, // 网络升级
		UPDATE_TYPE_SDCARD, // sd卡升级
		UPDATE_TYPE_CENTER, // 管理中心升级
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
		/* ---------------------------------------------------------------------------*/
		/**
		 * @brief int 是否需要升级 1是，0否
		 *
		 * @param 
		 * @param version 需要升级时，升级后版本
		 * @param content 升级内容
		 */
		/* ---------------------------------------------------------------------------*/
		int (*needUpdate)(struct _MyUpdate *,char *version,char *content);
	}MyUpdate;

	void myUpdateInit(void);
	void myUpdateStart(int type,char *ip,int port,char *file_path);

	extern MyUpdate *my_update;
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
