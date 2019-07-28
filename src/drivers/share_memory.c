/*
 * =============================================================================
 *
 *       Filename:  share_memory.c
 *
 *    Description:  共享内存管理
 *
 *        Version:  1.0
 *        Created:  2019-07-27 16:21:13
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
/* ---------------------------------------------------------------------------*
 *                      include head files
 *----------------------------------------------------------------------------*/
#include <stdio.h>
#include <malloc.h>
#include <string.h>

#include <fcntl.h>
#include <errno.h>
#include <semaphore.h>
#include <sys/ipc.h>
#include <sys/shm.h>
#include <sys/sem.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "share_memory.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define MAXBUFCNT 5
#define SEM_PRG_GET   "Prg_sem_get"
#define SEM_PRG_SAVE  "Prg_sem_save"

struct ProcessData {
    int size;
    char data[1];
};

union ShareMemoryProcessData {
    char *p;   
    struct ProcessData *process_buf;
};

typedef struct _ShareMemoryPrivate
{
	int Type;											// 0线程共享内存 非0进程共享内存
	int shmid;											// 共享内存id
	int GetSemId;										//读信号量
	int SaveSemId;										//写信号量
	int Terminate;										//结束
	int MallocSize;										//每段内存申请的大小
	unsigned int MallocCnt;								//分配多少段内存
	char * Buf[MAXBUFCNT];								//内存指针
    struct ProcessData *process_buf[MAXBUFCNT];         // 进程通信用该指针
	int BufSize[MAXBUFCNT];								//内存的数据量
	sem_t *GetSem;										//读信号量
	sem_t *SaveSem;										//写信号量
	int GetIndex;										//读索引，用户只读
	int SaveIndex;										//写索引，用户只读
	unsigned int WriteCnt;										//写入的缓冲区数量
}ShareMemoryPrivate,*PShareMemoryPrivate;

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *arry;
};


/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/


static int set_semvalue(int sem_id,int val)
{
	//用于初始化信号量，在使用信号量前必须这样做
	union semun sem_union;

	sem_union.val = val;
	if(semctl(sem_id, 0, SETVAL, sem_union) == -1)
		return 0;
	return 1;
}

static void del_semvalue(int sem_id)
{
    //删除信号量
	union semun sem_union;

	if(semctl(sem_id, 0, IPC_RMID, sem_union) == -1)
		fprintf(stderr, "F:delete semaphore:%s\n",strerror(errno));
}

static int semaphore_p(int sem_id)
{
	// 对信号量做减1操作，即等待P（sv）
	struct sembuf sem_b;
	sem_b.sem_num = 0;
	sem_b.sem_op = -1;//P()
	sem_b.sem_flg = SEM_UNDO;

	if(semop(sem_id, &sem_b, 1) == -1)  {
		fprintf(stderr, "F:semaphore_p failed:%s\n",strerror(errno));
		return -1;
	}
	return 0;
}

static int semaphore_v(int sem_id)
{
    //这是一个释放操作，它使信号量变为可用，即发送信号V（sv）
	struct sembuf sem_b;
	sem_b.sem_num = 0;
	sem_b.sem_op = 1;//V()
	sem_b.sem_flg = SEM_UNDO;
	if(semop(sem_id, &sem_b, 1) == -1)  {
		fprintf(stderr, "F:semaphore_v failed:%s\n",strerror(errno));
		return -1;
	}
	return 0;
}

/* ----------------------------------------------------------------*/
/**
 * 线程操作时信号量处理  
 * @brief my_sem_post_get  
 * @brief my_sem_post_save 
 * @brief my_sem_wait_get 
 * @brief my_sem_wait_save 
 *
 * @param This
 */
/* ----------------------------------------------------------------*/
static int thread_sem_post_get(PShareMemory This)
{
	return sem_post(This->Private->GetSem);
}
static int thread_sem_post_save(PShareMemory This)
{
	return sem_post(This->Private->SaveSem);
}
static int thread_sem_wait_get(PShareMemory This)
{
	return sem_wait(This->Private->GetSem);
}
static int thread_sem_wait_save(PShareMemory This)
{
	return sem_wait(This->Private->SaveSem);
}

/* ----------------------------------------------------------------*/
/**
 * 进程操作时信号量处理  
 * @brief my_sem_post_get  
 * @brief my_sem_post_save 
 * @brief my_sem_wait_get 
 * @brief my_sem_wait_save 
 *
 * @param This
 */
/* ----------------------------------------------------------------*/
static int process_sem_post_get(PShareMemory This)
{
	return semaphore_v(This->Private->GetSemId);
}
static int process_sem_post_save(PShareMemory This)
{
	return semaphore_v(This->Private->SaveSemId);
}
static int process_sem_wait_get(PShareMemory This)
{
	return semaphore_p(This->Private->GetSemId);
}
static int process_sem_wait_save(PShareMemory This)
{
	return semaphore_p(This->Private->SaveSemId);
}

//----------------------------------------------------------------------------
static void ShareMemory_CloseMemory(PShareMemory This)
{
	unsigned int i;
    This->Private->Terminate = 1;
    if (This->Private->Type == 0) {
        for(i=0;i<This->Private->MallocCnt;i++) {
            This->My_sem_post_get(This);
            This->My_sem_post_save(This);
        }
    } else {
        for (i=0; i<This->Private->MallocCnt; i++) {
            memset(This->Private->process_buf[i],0,This->Private->MallocSize); 
        }
		set_semvalue(This->Private->GetSemId,0);
		set_semvalue(This->Private->SaveSemId,This->Private->MallocCnt);
    }
}
//----------------------------------------------------------------------------
// static void ShareMemory_InitSem(PShareMemory This)
// {
	// unsigned int i;
	// for(i=0;i<This->Private->MallocCnt;i++) {
		// sem_post(This->Private->GetSem);			//解除阻塞
		// sem_post(This->Private->SaveSem);
	// }
// }
//----------------------------------------------------------------------------
static void ShareMemory_Destroy(PShareMemory This)
{
	unsigned int i;
	This->Private->Terminate = 1;
	if (This->Private->Type == 0) {
		sem_destroy(This->Private->GetSem);
		sem_destroy(This->Private->SaveSem);
		for(i=0;i<This->Private->MallocCnt;i++) {
			free(This->Private->Buf[i]);
			This->Private->Buf[i] = NULL;
		}
		free(This->Private->SaveSem);
		free(This->Private->GetSem);
	} else {
		del_semvalue(This->Private->GetSemId);
		del_semvalue(This->Private->SaveSemId);
		shmctl(This->Private->shmid, IPC_RMID,0);
	}
	free(This->Private);
	free(This);
}
//----------------------------------------------------------------------------
static unsigned int ShareMemory_WriteCnt(PShareMemory This)
{
	return This->Private->WriteCnt;
}
//----------------------------------------------------------------------------
static char * ShareMemory_SaveStart(PShareMemory This)
{

	if(This->Private == NULL || This->Private->Terminate)
		return NULL;
	else {
		This->My_sem_wait_save(This);
        if (This->Private->Type == 0)
            return This->Private->Buf[This->Private->SaveIndex];
        else
            return This->Private->process_buf[This->Private->SaveIndex]->data;
	}
}
//----------------------------------------------------------------------------
static void ShareMemory_SaveEnd(PShareMemory This,int Size)
{
    if(This->Private->WriteCnt<This->Private->MallocCnt)
        This->Private->WriteCnt++;

    if(Size>This->Private->MallocSize)
        Size = This->Private->MallocSize;

    if (This->Private->Type == 0)
        This->Private->BufSize[This->Private->SaveIndex]=Size;
    else
        This->Private->process_buf[This->Private->SaveIndex]->size=Size;

    This->My_sem_post_get(This);

    This->Private->SaveIndex = (This->Private->SaveIndex+1) % This->Private->MallocCnt;
}
//----------------------------------------------------------------------------
static char * ShareMemory_GetStart(PShareMemory This,int *Size)
{
	if(This->Private->Terminate || This->Private==NULL) {
		*Size = 0;
		return NULL;
	} else {
        if (This->My_sem_wait_get(This) == -1) {
            *Size = 0;
            return NULL;
        }

        if (This->Private->Type == 0) {
            *Size = This->Private->BufSize[This->Private->GetIndex];
            return This->Private->Buf[This->Private->GetIndex];
        } else {
            *Size = This->Private->process_buf[This->Private->GetIndex]->size;
            return This->Private->process_buf[This->Private->GetIndex]->data;
        }
    }
}
//----------------------------------------------------------------------------
static void ShareMemory_GetEnd(PShareMemory This)
{
    if(This->Private->WriteCnt)
        This->Private->WriteCnt--;
    This->My_sem_post_save(This);
    This->Private->GetIndex = (This->Private->GetIndex+1) % This->Private->MallocCnt;
}
//----------------------------------------------------------------------------
/* ----------------------------------------------------------------*/
/**
 * @brief CreateShareMemory 创建共享内存
 *
 * @param Size 内存大小
 * @param BufCnt 内存块数
 * @param ... 0为线程共享，非0为进程共享
 *
 * @returns
 */
/* ----------------------------------------------------------------*/
ShareMemory *CreateShareMemory(unsigned int Size,unsigned int BufCnt,int type)
{
	int i;

	PShareMemory This = (PShareMemory)calloc(sizeof(ShareMemory),1);
	if(This == NULL) {
		printf("F:Err: Creat This\n");
		return NULL;
	}

	This->Private = (PShareMemoryPrivate)calloc(sizeof(ShareMemoryPrivate),1);
	if(This->Private == NULL) {
		free(This);
		printf("F:Err: Creat This->Private\n");
		return NULL;
	}

	This->Private->Type = type;
	if (This->Private->Type == 0) {
		if(BufCnt > MAXBUFCNT) {
			BufCnt = MAXBUFCNT;
		}
		for(i=0;i<(int)BufCnt;i++) {
			This->Private->Buf[i] = (char *)malloc(Size);
			if(This->Private->Buf[i]==NULL) {
				i--;
				for(;i>=0;i--) {
					free(This->Private->Buf[i]);
				}
				printf("F:Err: Creat Buf\n");
				goto creat_err;
			}
		}
	} else {
		This->Private->shmid = shmget((key_t)1111,Size * BufCnt,IPC_CREAT | 0666);
		if (This->Private->shmid < 0) {
			fprintf(stderr, "F:Shmget:%s\n", strerror(errno));
			goto creat_err;
		}
        char *sharemem = (char *)shmat(This->Private->shmid,NULL,0);
		if (sharemem < 0) {
			fprintf(stderr, "F:shmat:%s\n", strerror(errno));
			goto creat_err;
		}
        for (i=0; i<(int)BufCnt; i++) {
            This->Private->process_buf[i] = (struct ProcessData*)(sharemem + i*Size);
            This->Private->process_buf[i]->size = 0;
        }
	}

	if (This->Private->Type == 0) {
		This->Private->GetSem = (sem_t *) malloc(sizeof(sem_t));
		This->Private->SaveSem = (sem_t *) malloc(sizeof(sem_t));
		sem_init (This->Private->GetSem, 0,0);					//读信号量初始化为0
		sem_init (This->Private->SaveSem, 0,BufCnt);			//写信号量初始化为缓冲区数量
		This->My_sem_post_get = thread_sem_post_get;
		This->My_sem_post_save = thread_sem_post_save;
		This->My_sem_wait_get = thread_sem_wait_get;
		This->My_sem_wait_save = thread_sem_wait_save;
	} else {
		This->Private->GetSemId = semget((key_t)1234, 1, 0666 | IPC_CREAT);//sem_open(SEM_PRG_GET,O_CREAT,0666,0);
		if (This->Private->GetSemId < 0) {
			fprintf(stderr, "F:sem_open get:%s\n", strerror(errno));
			goto creat_err;
		}
		This->Private->SaveSemId = semget((key_t)1236, 1, 0666 | IPC_CREAT);//sem_open(SEM_PRG_SAVE,O_CREAT,0666,BufCnt);
		if (This->Private->SaveSemId < 0) {
			fprintf(stderr, "F:sem_open save:%s\n", strerror(errno));
			goto creat_err;
		}
		set_semvalue(This->Private->GetSemId,0);
		set_semvalue(This->Private->SaveSemId,BufCnt);
		This->My_sem_post_get = process_sem_post_get;
		This->My_sem_post_save = process_sem_post_save;
		This->My_sem_wait_get = process_sem_wait_get;
		This->My_sem_wait_save = process_sem_wait_save;
	}

	This->Private->Terminate = 0;
	This->Private->MallocSize = Size;
	This->Private->MallocCnt = BufCnt;
	This->Private->GetIndex = 0;
	This->Private->SaveIndex = 0;
	This->Private->WriteCnt = 0;
	This->Destroy = ShareMemory_Destroy;
//	This->InitSem = ShareMemory_InitSem;
	This->CloseMemory = ShareMemory_CloseMemory;
	This->SaveStart = ShareMemory_SaveStart;
	This->SaveEnd = ShareMemory_SaveEnd;
	This->GetStart = ShareMemory_GetStart;
	This->GetEnd = ShareMemory_GetEnd;
	This->WriteCnt = ShareMemory_WriteCnt;

	return This;

creat_err:
	free(This->Private);
	free(This);
	return NULL;

}
//----------------------------------------------------------------------------
