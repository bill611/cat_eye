/*
 * =============================================================================
 *
 *       Filename:  ipc_server.h
 *
 *    Description:  进程通讯接口
 *
 *        Version:  virsion
 *        Created:  2019-07-26 11:18:51 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _IPC_SERVER_H
#define _IPC_SERVER_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */
	typedef void (*IpcCallback)(char *data,int size);

	struct _IpcServerPriv;
	typedef struct _IpcServer{
		struct _IpcServerPriv *priv;
		int (*sendData)(struct _IpcServer *,char *path,void *data,int size);

	}IpcServer;
	void waitIpcOpen(char *path);

	IpcServer* ipcCreate(const char *path,IpcCallback func);
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
