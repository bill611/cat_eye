/*
 * =============================================================================
 *
 *       Filename:  Queue.c
 *
 *    Description:  队列
 *
 *        Version:  1.0
 *        Created:  2016-08-16 09:08:15
 *       Revision:  linux 需要内核配置支持
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
/* ---------------------------------------------------------------------------*
 *                      include head files
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
#include <stdio.h>
#include <malloc.h>
#include <string.h>
#include <pthread.h>

#include <fcntl.h>
#include <errno.h>
#include <sys/types.h>
#include <semaphore.h>
#include <mqueue.h>
#include <sys/ipc.h>

#include <sys/msg.h>

#include "queue.h"


/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define QUEUE_LOCK()   pthread_mutex_lock(&This->priv->mutex)
#define QUEUE_UNLOCK() pthread_mutex_unlock(&This->priv->mutex)

typedef struct _QueuePriv {
	char name[32];
	QueueType type;
	unsigned int size;
	int queue;
	void *buf[MAX_COMMAND_QUEUE_SIZE];
	sem_t *sem;				//信号量

	int current_length; // 当前fifo已存储的长度
	int index_post;				//索引
	int index_get;				//索引

	pthread_mutex_t mutex;		//队列控制互斥信号
	pthread_mutexattr_t mutexattr2;

}QueuePriv;
/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/



/* ----------------------------------------------------------------*/
/**
 * 线程操作时信号量处理
 * @brief my_sem_post_get
 * @brief my_sem_wait_get
 *
 * @param This
 */
/* ----------------------------------------------------------------*/
static void semPost(Queue * This)
{
	sem_post(This->priv->sem);
}
static void semWait(Queue * This)
{
	sem_wait(This->priv->sem);
}

//----------------------------------------------------------------------------
static void destroy(Queue * This)
{
	if (This->priv->type == QUEUE_NONBLOCK) {
		mq_close(This->priv->queue);
		mq_unlink(This->priv->name);
	} else {
		unsigned int i;
		sem_destroy(This->priv->sem);
		for(i=0;i<MAX_COMMAND_QUEUE_SIZE;i++) {
			if (This->priv->buf[i]) {
				free(This->priv->buf[i]);
				This->priv->buf[i] = NULL;
			}
		}
		free(This->priv->sem);
		pthread_mutex_destroy (&This->priv->mutex);
	}
	free(This->priv);
	free(This);
}

//----------------------------------------------------------------------------
static void queuePost(Queue * This,void *data)
{
	if (This->priv->type == QUEUE_NONBLOCK) {
		if (msgsnd(This->priv->queue, (const char*)data, This->priv->size, IPC_NOWAIT))
			fprintf(stderr, "mq_send failed： %s\n", strerror(errno));
	} else {
		if (This->priv->current_length >= MAX_COMMAND_QUEUE_SIZE) {
			printf("fifo full!!%s\n",This->priv->name);
			return;
		}
		QUEUE_LOCK();
		if (This->priv->index_post >= MAX_COMMAND_QUEUE_SIZE) {
			This->priv->index_post = 0;
		}

		This->priv->buf[This->priv->index_post] = (void *) calloc (1,This->priv->size);
		memcpy(This->priv->buf[This->priv->index_post],data,This->priv->size);
		This->priv->index_post++;
		This->priv->current_length++;
		QUEUE_UNLOCK();
		semPost(This);
	}
}

static int queueGet(Queue *This,void *data)
{
	if (This->priv->type == QUEUE_NONBLOCK) {
		return msgrcv(This->priv->queue, (void *)data, This->priv->size, 0,IPC_NOWAIT);
		// return	mq_receive(This->priv->queue, (char*)data, This->priv->size, 0);
	}
	semWait(This);
	if (This->priv->current_length == 0) {
		printf("fifo empty!!\n");
		return 0;
	}
	QUEUE_LOCK();
	if (This->priv->index_get >= MAX_COMMAND_QUEUE_SIZE) {
		This->priv->index_get = 0;
	}
	memcpy(data,This->priv->buf[This->priv->index_get],This->priv->size);
	free(This->priv->buf[This->priv->index_get]);
	This->priv->buf[This->priv->index_get] = NULL;
	This->priv->index_get++;
	This->priv->current_length--;
	QUEUE_UNLOCK();
	return 0;
}

/* ----------------------------------------------------------------*/
/**
 * @brief queueCreate 创建对列
 *
 * @param Size 参数大小
 *
 * @returns
 */
/* ----------------------------------------------------------------*/
Queue * queueCreate(char *queue_name,QueueType type,unsigned int Size)
{
	Queue * This = (Queue *)calloc(1,sizeof(Queue));
	if(This == NULL) {
		printf("fifo calloc failed !!!\n");
		return NULL;
	}
	This->priv = (QueuePriv *)calloc(1,sizeof(QueuePriv));
	if(This == NULL) {
		printf("fifo priv calloc failed !!!\n");
		free(This);
		return NULL;
	}
	This->priv->type = type;
	if (This->priv->type == QUEUE_NONBLOCK) {
		// struct mq_attr attr;
		// attr.mq_flags = 0;
		// attr.mq_maxmsg = MAX_COMMAND_QUEUE_SIZE;
		// attr.mq_msgsize = Size;
		strcpy(This->priv->name,queue_name);
		This->priv->queue = msgget((key_t)1234,0666|IPC_CREAT);
		if (This->priv->queue < 0)
			fprintf(stderr, "mq_create failed： %s\n", strerror(errno));
	} else {
		//设置互斥锁属性
		pthread_mutexattr_init(&This->priv->mutexattr2);
		/* Set the mutex as a recursive mutex */
		pthread_mutexattr_settype(&This->priv->mutexattr2,
				PTHREAD_MUTEX_RECURSIVE_NP);
		// PTHREAD_MUTEX_RECURSIVE_NP);
		/* create the mutex with the attributes set */
		pthread_mutex_init(&This->priv->mutex, &This->priv->mutexattr2);
		/* destroy the attribute */
		pthread_mutexattr_destroy(&This->priv->mutexattr2);

		This->priv->sem = (sem_t *) calloc(1,sizeof(sem_t));
		sem_init (This->priv->sem, 0,0);			//读信号量初始化为0

		This->priv->current_length = 0;
		This->priv->index_get = 0;
		This->priv->index_post = 0;

	}
	This->priv->size = Size;

	This->post = queuePost;
	This->get = queueGet;
	This->destroy = destroy;

	return This;
}
