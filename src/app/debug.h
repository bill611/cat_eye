/*
 * =============================================================================
 *
 *       Filename:  debug.h
 *
 *    Description:  调试接口
 *
 *        Version:  1.0
 *        Created:  2016-11-26 22:38:57 
 *       Revision:  1.0
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _TC_DEBUG_H
#define _TC_DEBUG_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

#include <stdint.h>
#define NELEMENTS(array)		/* number of elements in an array */ \
		(sizeof (array) / sizeof ((array) [0]))

#define DPRINT(...)                                      \
do {                                                     \
    printf("\033[1;34;40m");  \
    printf(__VA_ARGS__);                                 \
    printf("\033[0m");                                   \
} while (0)
		
#define DBG_FLAG(x) DPRINT("flag------->[%ld]\n",x)
#define DBG_STR(x)  DPRINT("flag------->[%s]\n",x)

void debugInit(void);
void saveLog(char *fmt, ...);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
