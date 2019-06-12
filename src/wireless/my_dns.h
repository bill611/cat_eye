/*
 * =============================================================================
 *
 *       Filename:  Dns.h
 *
 *    Description:  域名解析
 *
 *        Version:  1.0
 *        Created:  2016-06-27 15:29:35 
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
#ifndef _MY_DNS_H
#define _MY_DNS_H

#ifdef __cplusplus
extern "C" {
#endif  /* __cplusplus */

	int dnsGetIp(char *domain_name,char *ip);
	// int dnsGetIp(char *domain_name,const char *ip);

#ifdef __cplusplus
}
#endif  /* __cplusplus */

#endif
