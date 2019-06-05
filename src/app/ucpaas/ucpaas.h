/*
 * =============================================================================
 *
 *       Filename:  ucpaas.h
 *
 *    Description:  云之讯对讲接口
 *
 *        Version:  1.0
 *        Created:  2019-06-05 17:28:39 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _UCPAAS_SDK_H
#define _UCPAAS_SDK_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

	void ucsDial(char *user_id,void (*callBack)(void *arg));

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
