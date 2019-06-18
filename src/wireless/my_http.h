/*
 * =============================================================================
 *
 *       Filename:  my_http.h
 *
 *    Description:  封装tcp/ip接口
 *
 *        Version:  1.0
 *        Created:  2019-05-21 11:32:52 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _MY_HTTP_H
#define _MY_HTTP_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

	typedef struct _MyHttp {
		int (*post)(char *url, char *para, char **out_data);
		int (*download)(char *url, char *para, char *file_path);
	}MyHttp;

	MyHttp * myHttpCreate(void);
#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
