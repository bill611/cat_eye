/*
 * =============================================================================
 *
 *       Filename:  share_memory.h
 *
 *    Description:  �����ڴ����
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
		unsigned int (*WriteCnt)(struct _ShareMemory *This);		//����д��Ļ���������
		char* (*SaveStart)(struct _ShareMemory *This);				//���д���
		void (*SaveEnd)(struct _ShareMemory *This,int Size);		//�ͷ�д�����SizeΪд���ֽ���
		char* (*GetStart)(struct _ShareMemory *This,int *Size);	//��ö������SizeΪ�������ֽ���
		void (*GetEnd)(struct _ShareMemory *This);					//�ͷŶ����
		//	void (* InitSem)(struct _ShareMemory *This);				//��ʼ���ź�
		void (* CloseMemory)(struct _ShareMemory *This);			//����
		void (* Destroy)(struct _ShareMemory *This);				//���ٸ�ShareMemory
		int (* My_sem_post_get)(struct _ShareMemory *This);
		int (* My_sem_post_save)(struct _ShareMemory *This);
		int (* My_sem_wait_get)(struct _ShareMemory *This);
		int (* My_sem_wait_save)(struct _ShareMemory *This);
	}ShareMemory,*PShareMemory;

	ShareMemory* CreateShareMemory(unsigned int Size,unsigned int BufCnt,int type);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif

