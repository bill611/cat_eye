/*
 * =============================================================================
 *
 *       Filename:  UDPServer.c
 *
 *    Description:  udp驱动
 *
 *        Version:  1.0
 *        Created:  2018-03-05 17:33:54
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
#include <unistd.h>
#include <fcntl.h>
#include <string.h>
#include <stdarg.h>
#include <sys/stat.h>

#include "thread_helper.h"
#include "externfunc.h"
#include "debug.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define	CONSOLE_PATH		"/tmp/consolename"


/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/

static int stdio_redirect(void)
{
	char tty[32] = {0};
	int fd=open(CONSOLE_PATH, O_RDWR);

	if (fd >= 0)
	{
		//获取要切换到的tty的名称
		read(fd, tty, sizeof(tty));

		printf("new console name %s \n", tty);

		close(fd);

		if((fd = open(tty, O_RDWR)) < 0)
		{
			printf("open file failed\r\n");
			return -1;
		}

		//复制fd的文件表项到标准输出
		dup2(fd, STDOUT_FILENO);
		close(fd);
	}

	return -1;
}

static void * threadredirect(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	printf("tty %s \n", ttyname(1));
	while(1)
	{
		if(access(CONSOLE_PATH, F_OK | R_OK | W_OK) >= 0) {
			stdio_redirect();
			unlink(CONSOLE_PATH);
		}
		sleep(2);
	}
	return 0;
}
/* ---------------------------------------------------------------------------*/
/**
 * @brief udpServerInit 初始化udp服务
 *
 * @param port 侦听端口
 */
/* ---------------------------------------------------------------------------*/
void debugInit(void)
{
	createThread(threadredirect,NULL);
}

void saveLog(char *fmt, ...)
{
    FILE *log_fd = NULL;
    struct stat stat_buf;
    stat("log.txt", &stat_buf) ;
    printf("log.txt size:%ld\n", stat_buf.st_size);

    va_list args;
    va_start(args, fmt);
    vfprintf(stdout,fmt,args);
    if (stat_buf.st_size > 3 * 1024 *1024)
        log_fd = fopen("log.txt","w");
    else
        log_fd = fopen("log.txt","ab+");
    if (log_fd) {
           char time_now[50];
           getDate(time_now,sizeof(time_now));
           fprintf(log_fd,"[%s]",time_now);
           vfprintf(log_fd,fmt,args);
           fflush(log_fd);
           fclose(log_fd);
        }
    va_end(args);
}
