/*
 * =============================================================================
 *
 *       Filename:  my_echo.h
 *
 *    Description:  消回音处理
 *
 *        Version:  1.0
 *        Created:  2019-08-05 10:33:29 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _MY_ECHO_H
#define _MY_ECHO_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

void rkEchoTx(short int *pshwIn,
	    short int *pshwRef,
		short int *pshwOut,
		int swFrmLen);
void rkEchoRx(short int *pshwIn, 
		short int *pshwOut, 
		int swFrmLen);

void rkEchoInit(void);
void rkEchoUnInit(void);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
