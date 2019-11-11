/*
 * =============================================================================
 *
 *       Filename:  stateMachine.h
 *
 *    Description:  状态机处理
 *
 *        Version:  1.0
 *        Created:  2016-11-25 14:49:31 
 *       Revision:  1.0
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _STATE_MACHINE_H
#define _STATE_MACHINE_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */
#define DBG_MACHINE 1

	typedef struct _StateTableDebug {
		char **ev;
		char **st;
		char **todo;
	}StateTableDebug;

	typedef struct _StateTable {
		int msg;			// 事件消息
		int cur_state;		// 当前状态
		int next_state;		// 下一状态
		int run;			// 执行处理
	}StateTable;

	struct _StMachinePriv;
	typedef struct _StMachine {
		struct _StMachinePriv *priv;
		int	id;		

		void  *(*initPara)(struct _StMachine *,int size);
		int  (*getCurrentstate)(struct _StMachine *);
		void  (*setCurrentstate)(struct _StMachine *,int state);
		int  (*getCurRun)(struct _StMachine *);
        /* ---------------------------------------------------------------------------*/
        /**
         * @brief 发送异步消息
         *
         * @param 
         * @param msg 消息值
         * @param data 附加数据，需要用initPara初始化
         */
        /* ---------------------------------------------------------------------------*/
		void  (*msgPost)(struct _StMachine *,int msg,void *data);
        /* ---------------------------------------------------------------------------*/
        /**
         * @brief  发送同步消息
         *
         * @param 
         * @param msg 消息值
         * @param data 附加数据，直接传入参数，无需初始化
         *
         * @returns -1为未找到当前消息对应执行动作，其他为执行动作返回值
         */
        /* ---------------------------------------------------------------------------*/
		int  (*msgPostSync)(struct _StMachine *,int msg,void *data);
		int  (*handle)(struct _StMachine *,int result,void *data,void *arg);
		void  (*destroy)(struct _StMachine **);
	}StMachine;

	StMachine* stateMachineCreate(int init_state, 
			StateTable *state_table, 
			int num,
			int id,
			int (*handle)(StMachine *,int result,void *data,void *arg),
			void *arg,
			StateTableDebug *st_debug);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
