/*
 * =============================================================================
 *
 *       Filename:  playvoice.c
 *
 *    Description:  播放音频
 *
 *        Version:  1.0
 *        Created:  2019-07-27 12:29:55
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
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <strings.h>
#include <unistd.h>

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define DPRINT(...)           \
do {                          \
    printf("\033[1;34m");  \
    printf("[UART->%s,%d]",__func__,__LINE__);   \
    printf(__VA_ARGS__);      \
    printf("\033[0m");        \
} while (0)


/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/

// static char   cmd_buf[1024] = {0};
char * excuteCmd(char *Cmd,...)
{
	int i;
	FILE *fp;
	int ret;
	va_list argp;
	char *argv;
	char commond[512] = {0};
	strcat(commond,Cmd);
	va_start( argp, Cmd);
	for(i=1;i<20;i++) {
		argv = va_arg(argp,char *);
		if(argv == NULL)
			break;
		strcat(commond," ");
		strcat(commond,argv);
	}
	va_end(argp);
	DPRINT("cmd :%s\n",commond);
#if 0
	if ((fp = popen(commond, "r") ) == 0) {
		perror("popen");
		return NULL;
	}
	memset(cmd_buf,0,sizeof(cmd_buf));
	ret = fread( cmd_buf, sizeof(cmd_buf), sizeof(char), fp ); //将刚刚FILE* stream的数据流读取到cmd_buf
	// DPRINT("r:%d\n",ret );
	if ( (ret = pclose(fp)) == -1 ) {
		DPRINT("close popen file pointer fp error!\n");
	}
	return cmd_buf;
#else
	// 用此函数导致产生僵尸进程
	 system(commond);
	 return 0;
#endif
}

int playVoice(char * file_name)
{
	excuteCmd("/data/play.sh",file_name,NULL);
}

