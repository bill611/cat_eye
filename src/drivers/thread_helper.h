/*
 * =============================================================================
 *
 *       Filename:  thread_helper.h
 *
 *    Description:  创建线程函数,自动销毁
 *
 *        Version:  1.0
 *        Created:  2019-05-25 14:05:07 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _THREAD_HELPER_H
#define _THREAD_HELPER_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <pthread.h>

	int createThread(void *(*start_routine)(void *), void *arg);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
