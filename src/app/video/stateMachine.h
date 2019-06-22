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
// #define DBG_MACHINE 1
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
		int  (*getCurRun)(struct _StMachine *);
		void  (*msgPost)(struct _StMachine *,int msg,void *data);
		void  (*handle)(struct _StMachine *,int result,void *data);
		void  (*destroy)(struct _StMachine **);
	}StMachine;

	StMachine* stateMachineCreate(int init_state, 
			StateTable *state_table, 
			int num,
			int id,
			void (*handle)(StMachine *,int result,void *data));

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
