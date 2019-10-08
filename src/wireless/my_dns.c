/*
 * =============================================================================
 *
 *       Filename:  Dns.c
 *
 *    Description:  域名解析
 *
 *        Version:  1.0
 *        Created:  2016-06-27 15:29:08
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =============================================================================
 */
/* ---------------------------------------------------------------------------*
 *                      include head files
 *----------------------------------------------------------------------------*/
#include <stdio.h>
#include <string.h>
#include <stdlib.h>

#include <signal.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <netinet/in.h>
#include <netinet/ip.h>
#include <netinet/ip_icmp.h>
#include <netdb.h>
#include <setjmp.h>
#include <errno.h>
#include "debug.h"
#include "my_dns.h"
/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/

int dnsGetIp(char *domain_name,char *ip)
{
#if 0
	struct hostent *host;
	struct sockaddr_in dest_addr;
	unsigned long inaddr=0l;
	if( (inaddr = inet_addr(domain_name)) == INADDR_NONE) {       
		if((host=gethostbyname(domain_name) ) == NULL) {
			perror("gethostbyname error");
			return -1;
		}
		memcpy( (char *)&dest_addr.sin_addr,host->h_addr,host->h_length);
		memcpy( ip,inet_ntoa(dest_addr.sin_addr),strlen(inet_ntoa(dest_addr.sin_addr)));
	} else {
		memcpy( ip,domain_name,strlen(domain_name));
	}
	return 0;
#else
	struct addrinfo * res, *pt;
	struct sockaddr_in *sinp;
	struct in_addr addr1;
	const char *addr;
	char abuf[INET_ADDRSTRLEN];
	int succ=0,i=0;
	if (strncmp("http://",domain_name,strlen("http://")) == 0) {
		domain_name += strlen("http://");
	} else if (strncmp("https://",domain_name,strlen("https://")) == 0) {
		domain_name += strlen("https://");
	}
	if (inet_pton(AF_INET, ip, &addr1) <= 0) {
		succ = getaddrinfo(domain_name, NULL, NULL, &res);
		if(succ != 0) {
			printf("dns fail,%s\n",domain_name );
			return -1;
		}
	

		for(pt=res, i=0; pt != NULL; pt=pt->ai_next, i++){
			sinp = (struct sockaddr_in *)pt->ai_addr;
			addr = inet_ntop(AF_INET, &sinp->sin_addr, abuf, INET_ADDRSTRLEN);
			// printf("%2d. IP=%s\n", i, addr);
		}
	} else {
		addr = domain_name;	
	}
	memcpy(ip,addr,strlen(addr));
	return 0;
#endif
}

