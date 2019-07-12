/*----------------------------------------------------------------------------
// Copyright by zhuhai taichang 2006.5.24
// Author: guanshangming
//
// �������ڴ���
// ����:
// PShareMemory pObject;				//����
// pObject = CreateShareMemory(Size);	//����һ�������ڴ�,Size��ʾÿ��������Buf�Ĵ�С
// pObject->Destroy(pObject);			//������CreateShareMemory�����Ĺ����ڴ�
// д������ʱ������SaveStart��ÿ�д����ڴ�����д�����ݺ���SaveEnd�ͷŸþ��
// ���������������SaveStart������ֱ������������ȡ���������
// ������д��������ؾ��������������٣�����NULL
// ���ͷ�д�������Ҫ���뻺�����������ֽ���
// pBuf = pObject->SaveStart(pObject);			//���д���
// pObject->SaveEnd(pObject,WriteSize);			//д��WriteSize�ֽ���,�ͷ�д���
// pBuf = pObject->GetStart(pObject,&BufSize);	//��ö����,BufSizeָʾ�������ֽ���
// pBuf = pObject->GetEnd(pObject);				//�ͷŶ����
//----------------------------------------------------------------------------*/

#ifndef ShareMemoryH
#define ShareMemoryH
#include <pthread.h>
#include <semaphore.h>

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
	void (* My_sem_post_get)(struct _ShareMemory *This);
	void (* My_sem_post_save)(struct _ShareMemory *This);
	void (* My_sem_wait_get)(struct _ShareMemory *This);
	void (* My_sem_wait_save)(struct _ShareMemory *This);
}ShareMemory,*PShareMemory;

PShareMemory CreateShareMemory(unsigned int Size,unsigned int BufCnt,int type);

#endif
