/*
 * =============================================================================
 *
 *       Filename:  my_ntp.h
 *
 *    Description:  同步时间接口
 *
 *        Version:  1.0
 *        Created:  2019-05-21 16:54:10 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _MY_NTPTIME_H
#define _MY_NTPTIME_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

	void ntpTime(char *server_ip,void (*callBack)(void));

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
