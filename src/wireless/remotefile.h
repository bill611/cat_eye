/*
 * =============================================================================
 *
 *       Filename:  remotefile.h
 *
 *    Description:  UDP下载文件
 *
 *        Version:  1.0
 *        Created:  2019-09-03 14:16:49 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _REMOTEFILE_H
#define _REMOTEFILE_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#ifndef FALSE
#define FALSE 0
#define TRUE 1
#define BOOL int
#endif

#include "my_update.h"
	enum {
		UPDATE_FAIL_REASON_CREATE,
		UPDATE_FAIL_REASON_CONNECT,
		UPDATE_FAIL_REASON_FILENAME,
		UPDATE_FAIL_REASON_RECV,
		UPDATE_FAIL_REASON_RECVSIZE,
		UPDATE_FAIL_REASON_OPEN,
		UPDATE_FAIL_REASON_RENAME,
		UPDATE_FAIL_REASON_ABORT,
	};

	enum {
		OPEN_READ=1,
		OPEN_WRITE,
		OPEN_READWRITE,
		FILE_CREATE_ALWAYS=0x100,
		FILE_CREATE_NEW=0x200,
		FILE_OPEN_ALWAYS=0x300,
		FILE_OPEN_EXISTING=0x400,
		FILE_OPEN_MASK=0xFF00
	};
	//---------------------------------------------------------------------------
	struct RemoteFilePrivate;        //私有数据

	typedef struct _TRemoteFile
	{
		struct RemoteFilePrivate * Private;
		BOOL (*Download)(struct _TRemoteFile* This,unsigned int hWnd,const char *SrcFile,
				const char *DstFile,int ExecuteMode,UpdateFunc callback);						//下载文件
		int (*Open)(struct _TRemoteFile* This,const char *FileName,int OpenMode);				//返回文件长度,-1打开失败
		int (*Read)(struct _TRemoteFile* This,void *Buffer,int Size);
		BOOL (*Write)(struct _TRemoteFile* This,void *Buffer,int Size);
		int (*Seek)(struct _TRemoteFile* This,int offset,int origin);
		BOOL (*SetEnd)(struct _TRemoteFile* This);
		int (*GetPos)(struct _TRemoteFile* This);
		void (*Close)(struct _TRemoteFile* This);
		void (*Destroy)(struct _TRemoteFile* This);
	} TRemoteFile;

	TRemoteFile * CreateRemoteFile(const char *IP,const char *TempFile);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif

