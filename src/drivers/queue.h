/*
 * =============================================================================
 *
 *       Filename:  Queue.h
 *
 *    Description:  队列
 *
 *        Version:  1.0
 *        Created:  2016-08-16 09:16:52
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _QUEUE_H
#define _QUEUE_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

// #define LINUX_QUENE 
#define MAX_COMMAND_QUEUE_SIZE 32

	typedef enum QueueType {
		QUEUE_NONBLOCK,
		QUEUE_BLOCK,
	}QueueType;
	struct _QueuePriv;
	typedef struct _Queue {
		struct _QueuePriv *priv;
		void (*post)(struct _Queue *,void *data);
		int (*get)(struct _Queue *,void *data);

		void (*destroy)(struct _Queue *This);
	}Queue;

	Queue * queueCreate(const char *queue_name,
			QueueType type,
			unsigned int Size);


#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
