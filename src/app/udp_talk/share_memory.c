/*----------------------------------------------------------------------------
// Copyright by zhuhai taichang 2006.5.25
// Author: guanshangming
//
// 建立线程之间的共享内存区
//
//----------------------------------------------------------------------------*/

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

#define MAXBUFCNT 5
#define SEM_PRG_GET   "Prg_sem_get"
#define SEM_PRG_SAVE  "Prg_sem_save"


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
	int BufSize[MAXBUFCNT];								//内存的数据量
	sem_t *GetSem;										//读信号量
	sem_t *SaveSem;										//写信号量
	int GetIndex;										//读索引，用户只读
	int SaveIndex;										//写索引，用户只读
	unsigned int WriteCnt;										//写入的缓冲区数量
	pthread_mutex_t mutex;		//队列控制互斥信号
	pthread_mutexattr_t mutexattr2;
}ShareMemoryPrivate,*PShareMemoryPrivate;

union semun
{
    int val;
    struct semid_ds *buf;
    unsigned short *arry;
};

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
		return 0;
	}
	return 1;
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
		return 0;
	}
	return 1;
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
static void thread_sem_post_get(PShareMemory This)
{
	sem_post(This->Private->GetSem);
}
static void thread_sem_post_save(PShareMemory This)
{
	sem_post(This->Private->SaveSem);
}
static void thread_sem_wait_get(PShareMemory This)
{
	sem_wait(This->Private->GetSem);
}
static void thread_sem_wait_save(PShareMemory This)
{
	sem_wait(This->Private->SaveSem);
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
static void process_sem_post_get(PShareMemory This)
{
	semaphore_v(This->Private->GetSemId);
	// sem_post(This->Private->GetSem);
}
static void process_sem_post_save(PShareMemory This)
{
	semaphore_v(This->Private->SaveSemId);
	// sem_post(This->Private->SaveSem);
}
static void process_sem_wait_get(PShareMemory This)
{
	semaphore_p(This->Private->GetSemId);
	// sem_wait(This->Private->GetSem);
}
static void process_sem_wait_save(PShareMemory This)
{
	semaphore_p(This->Private->SaveSemId);
	// sem_wait(This->Private->SaveSem);
}

//----------------------------------------------------------------------------
static void ShareMemory_CloseMemory(PShareMemory This)
{
	unsigned int i;
	pthread_mutex_lock (&This->Private->mutex);		//加锁
	if(This) {
		This->Private->Terminate = 1;
		for(i=0;i<This->Private->MallocCnt;i++)    {//Jack : no need to loop
			This->My_sem_post_get(This);
			This->My_sem_post_save(This);
			// sem_post(This->Private->GetSem);			//解除阻塞
			// sem_post(This->Private->SaveSem);
		}
	}
	pthread_mutex_unlock (&This->Private->mutex);		//解锁
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
		// sem_close(This->Private->GetSem);
		// sem_unlink(SEM_PRG_GET);
		// sem_close(This->Private->SaveSem);
		// sem_unlink(SEM_PRG_SAVE);
		shmctl(This->Private->shmid, IPC_RMID,0);
	}
	pthread_mutex_destroy (&This->Private->mutex);
	free(This->Private);
	This->Private = NULL;
	free(This);
	This = NULL;
}
//----------------------------------------------------------------------------
static unsigned int ShareMemory_WriteCnt(PShareMemory This)
{
	return This->Private->WriteCnt;
}
//----------------------------------------------------------------------------
static char * ShareMemory_SaveStart(PShareMemory This)
{

	//sem_wait (&This->Private->SaveSem);
	if(This->Private == NULL || This->Private->Terminate)
		return NULL;
	else {
		//pthread_mutex_lock (&This->Private->mutex);		//加锁
		This->My_sem_wait_save(This);
		// sem_wait (This->Private->SaveSem);//add By Jack
		return This->Private->Buf[This->Private->SaveIndex];
	}
}
//----------------------------------------------------------------------------
static void ShareMemory_SaveEnd(PShareMemory This,int Size)
{
    // printf("%s(),cnt:%d\n",__FUNCTION__,This->Private->WriteCnt);
	if(This->Private) {
		if(This->Private->WriteCnt<This->Private->MallocCnt)
			This->Private->WriteCnt++;
		if(Size>This->Private->MallocSize)
			Size = This->Private->MallocSize;
		This->Private->BufSize[This->Private->SaveIndex]=Size;

		This->My_sem_post_get(This);
		// sem_post (This->Private->GetSem);  //GetSem -->  SaveSem

		This->Private->SaveIndex = (This->Private->SaveIndex+1) % This->Private->MallocCnt;
		// printf("F:[%s]saveindex:%d,size:%d\n",
				// __FUNCTION__,
				// This->Private->SaveIndex,
				// This->Private->BufSize[This->Private->SaveIndex]);
		//pthread_mutex_unlock (&This->Private->mutex);
	}
}
//----------------------------------------------------------------------------
static char * ShareMemory_GetStart(PShareMemory This,int *Size)
{
	/*if(This->Private->Terminate)
	{
		*Size = 0;
		return NULL;
	}*/
	//sem_wait (&This->Private->GetSem);
	if(This->Private->Terminate || This->Private==NULL) {
		*Size = 0;
		return NULL;
	} else {
		//pthread_mutex_lock (&This->Private->mutex);		//加锁
		This->My_sem_wait_get(This);
		// sem_wait (This->Private->GetSem);  //modified by Jack

		*Size = This->Private->BufSize[This->Private->GetIndex];
		//printf("Jack: %s getindex = %d\n",__FUNCTION__,This->Private->GetIndex);
		return This->Private->Buf[This->Private->GetIndex];
	}
}
//----------------------------------------------------------------------------
static void ShareMemory_GetEnd(PShareMemory This)
{
    // printf("%s(),cnt:%d\n",__FUNCTION__,This->Private->WriteCnt);
	if(This->Private) {
		if(This->Private->WriteCnt)
			This->Private->WriteCnt--;
		This->My_sem_post_save(This);
		// sem_post (This->Private->SaveSem);   //SaveSem  ---> GetSem

		This->Private->GetIndex = (This->Private->GetIndex+1) % This->Private->MallocCnt;
		//printf("Jack: %s getindex = %d\n",__FUNCTION__,This->Private->GetIndex);
		//pthread_mutex_unlock (&This->Private->mutex);		//
	}
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
PShareMemory CreateShareMemory(unsigned int Size,unsigned int BufCnt,int type)
{
	int i;
//	pthread_mutexattr_t mutexattr2;

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
		BufCnt = 1;
		This->Private->shmid = shmget((key_t)1111,Size,IPC_CREAT | 0666);
		if (This->Private->shmid < 0) {
			fprintf(stderr, "F:Shmget:%s\n", strerror(errno));
			goto creat_err;
		}
		This->Private->Buf[0] = (char *)shmat(This->Private->shmid,NULL,0);
		if (This->Private->Buf[0] < 0) {
			fprintf(stderr, "F:shmat:%s\n", strerror(errno));
			goto creat_err;
		}
	}

	//设置互斥锁属性
	pthread_mutexattr_init(&This->Private->mutexattr2);
	/* Set the mutex as a recursive mutex */
	pthread_mutexattr_settype(&This->Private->mutexattr2, PTHREAD_MUTEX_RECURSIVE_NP);
	/* create the mutex with the attributes set */
	pthread_mutex_init(&This->Private->mutex, &This->Private->mutexattr2);
	/* destroy the attribute */
	pthread_mutexattr_destroy(&This->Private->mutexattr2);

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
