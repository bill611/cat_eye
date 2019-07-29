/*
 * =============================================================================
 *
 *       Filename:  share_memory.h
 *
 *    Description:  共享内存管理
 *
 *        Version:  1.0
 *        Created:  2019-07-27 16:20:04 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _SHARE_MEMORY_H
#define _SHARE_MEMORY_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <pthread.h>
#include <semaphore.h>

    typedef struct _ShareMemoryData {
        int size;
        char data[1];
    }ShareMemoryData;
	struct _ShareMemoryPrivate;

	typedef struct _ShareMemory
	{
		struct _ShareMemoryPrivate *Private;
		unsigned int (*WriteCnt)(struct _ShareMemory *This);		//返回写入的缓冲区数量
		char* (*SaveStart)(struct _ShareMemory *This);				//获得写句柄
		void (*SaveEnd)(struct _ShareMemory *This,int Size);		//释放写句柄，Size为写入字节数
		char* (*GetStart)(struct _ShareMemory *This,int *Size);	//获得读句柄，Size为缓冲区字节数
		void (*GetEnd)(struct _ShareMemory *This);					//释放读句柄
		//	void (* InitSem)(struct _ShareMemory *This);				//初始化信号
		void (* CloseMemory)(struct _ShareMemory *This);			//结束
		void (* Destroy)(struct _ShareMemory *This);				//销毁该ShareMemory
		int (* My_sem_post_get)(struct _ShareMemory *This);
		int (* My_sem_post_save)(struct _ShareMemory *This);
		int (* My_sem_wait_get)(struct _ShareMemory *This);
		int (* My_sem_wait_save)(struct _ShareMemory *This);
	}ShareMemory,*PShareMemory;

	ShareMemory* CreateShareMemory(unsigned int Size,unsigned int BufCnt,int type);
	ShareMemory * shareMemoryCreateMaster(unsigned int Size,unsigned int BufCnt);
	ShareMemory * shareMemoryCreateSlave(unsigned int Size,unsigned int BufCnt);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif

