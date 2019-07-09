/* * =============================================================================
 *
 *       Filename:  StateMachine.c
 *
 *    Description:  状态机
 *
 *        Version:  1.0
 *        Created:  2016-03-09 11:22:34
 *       Revision:  1.0
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <pthread.h>
#include "queue.h"
#include "debug.h"
#include "thread_helper.h"
#include "state_machine.h"

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#if DBG_MACHINE > 0
	#define DBG_P( ... ) DPRINT( __VA_ARGS__ )
#else
	#define DBG_P(...)
#endif

#define MAX_MSG 	5
#define TSK_BUSY    0x80
#define TSK_READY   0x01

typedef struct _MsgData {
	int msg;		
	void *data;
}MsgData;

typedef struct _StMachinePriv {
	Queue *queue;
    pthread_mutex_t mutex;
	StateTable *funcentry;
	StateTableDebug *st_debug;
	void *arg;
	int cur_state;
	int status_run;	
	int table_num;			
}StMachinePriv;
/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/
static int stmExecEntry(StMachine *This,int msg);

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
/* ---------------------------------------------------------------------------*/
/**
 * @brief stmMsgPost 状态机发送事件消息
 *
 * @param This
 * @param msg 消息
 * @param data 附加参数
 */
/* ---------------------------------------------------------------------------*/
static void stmMsgPost(StMachine* This,int msg,void *data)
{
	MsgData   msg_data;
	msg_data.msg = msg;
	msg_data.data = data;
	This->priv->queue->post(This->priv->queue,&msg_data);
}

static int stmMsgPostSync(StMachine* This,int msg,void *data)
{
    int ret = -1;
	pthread_mutex_lock(&This->priv->mutex);
    if (stmExecEntry(This,msg)) {
		printf("%s\n",__func__ );
        ret = This->handle(This,1,data,This->priv->arg);
    }
	pthread_mutex_unlock(&This->priv->mutex);
    return ret;
}


/* ---------------------------------------------------------------------------*/
/**
 * @brief stmExecEntry 状态机执行状态判断和转换
 *
 * @param This
 * @param msg 消息
 *
 * @returns 1成功 0失败
 */
/* ---------------------------------------------------------------------------*/
static int stmExecEntry(StMachine *This,int msg)
{
	int num = This->priv->table_num;
	StateTable *funcentry = This->priv->funcentry;

    for (; num > 0; num--,funcentry++) {
		if (		(msg == funcentry->msg) 
				&& 	(This->priv->cur_state == funcentry->cur_state)) {
			if (This->priv->st_debug) {
				DBG_P("[ST->msg:%s,cur:%s,next:%s,do:%s]\n",
						This->priv->st_debug->ev[msg],
						This->priv->st_debug->st[funcentry->cur_state],
						This->priv->st_debug->st[funcentry->next_state],
						This->priv->st_debug->todo[funcentry->run]);
			}
			This->priv->status_run = funcentry->run ;
			This->priv->cur_state = funcentry->next_state;
			return 1;
		}
	}
    return 0;
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief stmInitPara 初始化发送状态机消息时带的参数
 *
 * @param This
 * @param size 参数大小
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
static void *stmInitPara(StMachine *This,int size)
{
	void *para = NULL;
	if (size) {
		para = (void *) calloc (1,size);
	}
	return para;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief stmGetCurrentState 获取状态机当前状态
 *
 * @param This
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
static int stmGetCurrentState(StMachine *This)
{
	return This->priv->cur_state;	
}

static int stmGetCurRun(StMachine *This)
{
	return This->priv->status_run;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief stmDestroy 销毁状态机
 *
 * @param This
 */
/* ---------------------------------------------------------------------------*/
static void stmDestroy(StMachine **This)
{
	if ((*This)->priv)
		free((*This)->priv);
	if (*This)
		free(*This);
	*This = NULL;	
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief stmThread 状态机处理
 *
 * @param arg
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
static void *stmThread(void *arg)
{
    StMachine *stm= (StMachine *)arg;
	MsgData   msg_data;
    while(1) {
		memset(&msg_data,0,sizeof(MsgData));
		stm->priv->queue->get(stm->priv->queue,&msg_data);
        pthread_mutex_lock(&stm->priv->mutex);
		if (stmExecEntry(stm,msg_data.msg))
			stm->handle(stm,1,msg_data.data,stm->priv->arg);
		else
			stm->handle(stm,0,&msg_data.msg,stm->priv->arg);
		if (msg_data.data)
			free(msg_data.data);
        pthread_mutex_unlock(&stm->priv->mutex);
    }
    pthread_exit(NULL);
    return NULL;
}


/* ---------------------------------------------------------------------------*/
/**
 * @brief stateMachineCreate 创建状态机
 *
 * @param init_state 初始状态
 * @param state_table 状态机表
 * @param num 状态机表的数量
 * @param id 状态机的ID，区别多个状态机同时运行
 * @param handle 状态机处理，
 *
 * @returns 
 */
/* ---------------------------------------------------------------------------*/
StMachine* stateMachineCreate(int init_state, 
		StateTable *state_table, 
		int num,
		int id,
		int (*handle)(StMachine *This,int result,void *data,void *arg),
		void *arg,
		StateTableDebug *st_debug)
{

	StMachine *This = (StMachine *)calloc(1,sizeof(StMachine));
	This->priv = (StMachinePriv *)calloc(1,sizeof(StMachinePriv));
	This->priv->arg = arg;
	This->priv->funcentry = (StateTable *)state_table;
	This->priv->table_num = num;
	This->priv->cur_state = init_state;
	This->priv->queue = queueCreate("stm",QUEUE_BLOCK,sizeof(MsgData));
	This->priv->st_debug = st_debug;

	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&This->priv->mutex, &mutexattr);

	This->id = id;

	This->msgPost = stmMsgPost;
	This->msgPostSync = stmMsgPostSync;
	This->handle = handle;
	This->initPara = stmInitPara;
	This->getCurrentstate = stmGetCurrentState;
	This->getCurRun = stmGetCurRun;
	This->destroy = stmDestroy;
	createThread(stmThread,This);
	return This;
}

