#ifndef THREAD_HELPER_H
#define THREAD_HELPER_H

#include <pthread.h>

int createThread(void *(*start_routine)(void *), void *arg);

#endif  /* THREAD_HELPER_H */
