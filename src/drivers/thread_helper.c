#include <stdio.h>
#include "thread_helper.h"

int createThread(void *(*start_routine)(void *), void *arg)
{
	int result;
    pthread_t      task;
    pthread_attr_t attr;

    pthread_attr_init(&attr);
    pthread_attr_setdetachstate(&attr, PTHREAD_CREATE_DETACHED);
    result = pthread_create(&task, &attr, start_routine, arg);
	if(result)
		printf("create thread failt,Error code:%d\n",result);
	pthread_attr_destroy(&attr);
	return task;
}

