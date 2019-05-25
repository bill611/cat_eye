/*
 * =====================================================================================
 *
 *       Filename:  externfunc.c
 *
 *    Description:  外部函数
 *
 *        Version:  1.0
 *        Created:  2015-12-11 11:56:30
 *       Revision:  none
 *
 *         Author:  xubin
 *        Company:  Taichuan
 *
 * =====================================================================================
 */
/* ----------------------------------------------------------------*
 *                      include head files
 *-----------------------------------------------------------------*/
#include <stdio.h>
#include <stdlib.h>
#include <stdarg.h>
#include <string.h>
#include <time.h>

#include <strings.h>
#include <sys/socket.h>
#include <sys/ioctl.h>
#include <netinet/ip.h>
#include <net/if.h>
#include <fcntl.h>
#include <dirent.h>
#include <unistd.h>
#include <getopt.h>
#include <sys/time.h>
#include <sys/types.h>
#include <sys/stat.h>
#include <sys/wait.h>
#include <signal.h>
#include <linux/rtc.h>
#include <linux/fb.h>
#include <linux/types.h>
#include <linux/watchdog.h>
#include <sys/statfs.h>
#include <sys/mman.h>

#include "debug.h"
#include "thread_helper.h"
#include "externfunc.h"
#include "config.h"

/* ----------------------------------------------------------------*
 *                  extern variables declare
 *-----------------------------------------------------------------*/

/* ----------------------------------------------------------------*
 *                  internal functions declare
 *-----------------------------------------------------------------*/

/* ----------------------------------------------------------------*
 *                        macro define
 *-----------------------------------------------------------------*/
#define	IOCTL_PD	0x1001
#define	IOCTL_PU	0x1002
#define	IOCTL_NM	0x1003

/* ----------------------------------------------------------------*
 *                      variables define
 *-----------------------------------------------------------------*/
static void (*wifiloadCallback)(void *ap_info,int ap_cnt);
static int wifi_scan_end = 1;



/* ---------------------------------------------------------------------------*/
/**
 * @brief GetDate 取当前日期时间格式
 *
 * @param cBuf
 * @param Size
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
char * GetDate(char *cBuf,int Size)
{
	time_t timer;
    struct tm *tm1;
	if(Size<20) {
		if(cBuf) cBuf[0]=0;
		return cBuf;
	}
	timer = time(&timer);
	tm1 = localtime(&timer);
	sprintf(cBuf,
			"%04d-%02d-%02d %02d:%02d:%02d",
			tm1->tm_year+1900,
			tm1->tm_mon+1,
			tm1->tm_mday,
			tm1->tm_hour,
			tm1->tm_min,
			tm1->tm_sec);
	return cBuf;
}
struct tm * getTime(void)
{
	time_t timer;
	timer = time(&timer);
	return localtime(&timer);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief getMs 获得当前系统毫秒数
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
uint64_t getMs(void)
{
	struct  timeval    tv;
    gettimeofday(&tv,NULL);
	return ((tv.tv_usec / 1000) + tv.tv_sec  * 1000 );

}

/* ---------------------------------------------------------------------------*/
/**
 * @brief fileexists 判断文件是否存在
 *
 * @param FileName 文件名
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
int fileexists(const char *FileName)
{
	int ret = access(FileName,0);
	return ret == 0;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief strupper 小写转大写
 *
 * @param pdst
 * @param pstr
 * @param Size
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
char *strupper(char *pdst,const char *pstr,int Size)
{
	char *pchar = pdst;
	if(pstr==NULL)
		return NULL;
	strncpy(pdst,pstr,Size);
	while(*pchar) {
		if(*pchar>='a' && *pchar<='z')
			*pchar = *pchar - 'a' + 'A';
		pchar++;
	}
	return pdst;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief DelayMs 线程使用 延时毫秒
 *
 * @param ms
 */
/* ---------------------------------------------------------------------------*/
void DelayMs(int ms)
{
#if 0
	unsigned long long start_time;
	start_time = getMs();
	while (!(getMs() - start_time >= ms)) ;
#else
	usleep(ms*1000);
#endif
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief adjustdate 设置系统时间
 *
 * @param year
 * @param mon
 * @param mday
 * @param hour
 * @param min
 * @param sec
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
int adjustdate(int year,int mon,int mday,int hour,int min,int sec)
{
	//设备系统时间
	int rtc;
	time_t t;
	struct tm nowtime;
	nowtime.tm_sec=sec;			/*   Seconds.[0-60]   (1   leap   second)*/
	nowtime.tm_min=min;			/*   Minutes.[0-59]		*/
	nowtime.tm_hour=hour;		/*   Hours. [0-23]		*/
	nowtime.tm_mday=mday;		/*   Day.[1-31]			*/
	nowtime.tm_mon=mon-1;		/*   Month. [0-11]		*/
	nowtime.tm_year=year-1900;	/*   Year-   1900.		*/
	nowtime.tm_isdst=-1;		/*   DST.[-1/0/1]		*/
	t=mktime(&nowtime);
	stime(&t);

	//设置实时时钟
	rtc = open("/dev/rtc0",O_WRONLY);
	if(rtc<0) {
		rtc = open("/dev/misc/rtc",O_WRONLY);
		if(rtc<0) {
			DPRINT("can't open /dev/misc/rtc\n");
			return -1;
		}
	}
	if (ioctl( rtc, RTC_SET_TIME, &nowtime) < 0 ) {
		DPRINT("Could not set the RTC time\n");
		close(rtc);
		return -1;
	}
	close(rtc);
	return 0;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief recoverData 恢复数据
 *
 * @param file 文件名称
 * @param reboot 1需要重启 0不需要重启
 */
/* ---------------------------------------------------------------------------*/
int recoverData(const char *file)
{
	int size = strlen(file);
	char *backfile = (char *) malloc (sizeof(char) * size + 5);
	sprintf(backfile,"%s_bak",file);
	if (fileexists(backfile)) {
		excuteCmd("cp",backfile,file,NULL);
		sync();
		free(backfile);
		return 1;
	}
	free(backfile);
	return 0;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief backData   备份数据
 *
 * @param file
 */
/* ---------------------------------------------------------------------------*/
void backData(char *file)
{
	int size = strlen(file);
	char *backfile = (char *) malloc (sizeof(char) * size + 5);
	sprintf(backfile,"%s_bak",file);
	excuteCmd("cp",file,backfile,NULL);
	sync();
	free(backfile);
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief excuteCmd 执行shell命令,以NULL结尾
 *
 * @param Cmd
 * @param ...
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
static char   cmd_buf[1024] = {0};
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
	if ((fp = popen(commond, "r") ) == 0) {
		perror("popen");
		return NULL;
	}
#if 0
	int fd = fileno(fp);
	int flags = fcntl(fd, F_GETFL, 0);
	flags |= O_NONBLOCK;
	fcntl(fd, F_SETFL, flags);
#endif
	memset(cmd_buf,0,sizeof(cmd_buf));
	ret = fread( cmd_buf, sizeof(cmd_buf), sizeof(char), fp ); //将刚刚FILE* stream的数据流读取到cmd_buf
	// DPRINT("r:%d\n",ret );
	if ( (ret = pclose(fp)) == -1 ) {
		DPRINT("close popen file pointer fp error!\n");
	}
	return cmd_buf;

	// 用此函数导致产生僵尸进程
	// system(commond);
	// return 0;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief GetSendIP 判断本地IP与目标IP是否同一个网段,
 * 如果不在同一个网段将返回最小的广播域
 *
 * @param pSrcIP 本地IP地址
 * @param pDestIP 目标IP地址
 * @param pMask 本机子网掩码
 *
 * @returns 最小的广播域IP地址
 */
/* ---------------------------------------------------------------------------*/
const char * GetSendIP(const char *pSrcIP,const char *pDestIP,const char *pMask)
{
	//取广播域
	static char cIP[16];
	unsigned int SrcIP,DestIP,Mask;
	unsigned char *pIP = (unsigned char *)&DestIP;
	//转换字符形IP到整形IP地址
	SrcIP = inet_addr(pSrcIP);
	DestIP = inet_addr(pDestIP);
	Mask = inet_addr(pMask);
	if((SrcIP & Mask)!=(DestIP & Mask)) {
		DestIP = (SrcIP & Mask) | (~Mask & 0xFFFFFFFF);//DestIP = 0xFFFFFFFF;
	}
	//转换成字符形IP地址
	sprintf(cIP,"%d.%d.%d.%d",pIP[0],pIP[1],pIP[2],pIP[3]);
	return cIP;
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief jugdeRecIP 判断目标IP和本机IP是否在同一网段
 *
 * @param pSrcIP 本机IP
 * @param pDestIP 目标IP
 * @param pMask 本机掩码
 *
 * @returns 0不在同一网段 1在同一网段
 */
/* ---------------------------------------------------------------------------*/
int jugdeRecIP(const char *pSrcIP,const char *pDestIP,const char *pMask)
{
	unsigned int SrcIP,DestIP,Mask;
	//转换字符形IP到整形IP地址
	SrcIP = inet_addr(pSrcIP);
	DestIP = inet_addr(pDestIP);
	Mask = inet_addr(pMask);
	if((SrcIP & Mask)!=(DestIP & Mask))
		return 0;

	return 1;
}
/* ----------------------------------------------------------------*/
/**
 * @brief GetFileSize 获得文件大小
 *
 * @param file	文件名
 *
 * @returns
 */
/* ----------------------------------------------------------------*/
int GetFileSize(char *file)
{
	struct stat stat_buf;
	stat(file, &stat_buf) ;
	return stat_buf.st_size;
}

time_t MyGetTickCount(void)
{
	return time(NULL)*1000;
}

/* ----------------------------------------------------------------*/
/**
 * @brief print_data 格式化打印数据
 *
 * @param data 数据内容
 * @param len 长度
 */
/* ----------------------------------------------------------------*/
void print_data(char *data,int len)
{
	int i;
	for (i = 0; i < len; ++i) {
		if (i) {
			DPRINT("[%02d]0x%02x ",i,data[i] );
			if ((i%5) == 0)
				DPRINT("\n");
		} else {
			DPRINT("\n");
		}
	}
	DPRINT("\n");
}

/* ----------------------------------------------------------------*/
/**
 * @brief GetFilesNum 获取文件夹目录文件名及返回文件数目
 *
 * @param pPathDir 文件夹目录
 * @param func 每获得一个文件处理函数
 *
 * @returns 文件夹目录下的文件数目
 */
/* ----------------------------------------------------------------*/
int GetFilesNum(char *pPathDir,void (*func)(void *))
{
	// int i=0 ;
	// DIR *dir = NULL;
    // struct dirent *dirp = NULL;
	// struct _st_dir temp_file; // 保存文件名结构体
	// if((dir=opendir(pPathDir)) == NULL) {
		// DPRINT("Open File %s Error %s\n",pPathDir,strerror(errno));
		// return 0;
    // }

	// while((dirp=readdir(dir)) != NULL) {
		// if ((strcmp(".",dirp->d_name) == 0) || (strcmp("..",dirp->d_name) == 0)) {
			// continue;
		// }
		// i++;
		// sprintf(temp_file.path,"%s/%s",pPathDir,dirp->d_name);
		// if (func) {
			// func(&temp_file);
		// }
		// // DPRINT("i:%d,name:%s\n",i,dirp->d_name);
	// }
	// closedir(dir);
	// return i;
}

/* ----------------------------------------------------------------*/
/**
 * @brief ClearFramebuffer 清除fb0
 */
/* ----------------------------------------------------------------*/
void ClearFramebuffer(void)
{
	int fd = -1;
	unsigned int VpostWidth=0, VpostHeight=0,VpostBpp=0;
	unsigned int g_fb_vaddress=0;
	unsigned int g_u32VpostBufMapSize=0;
	struct	fb_var_screeninfo g_fb_var;
	fd = open("/dev/fb0", O_RDWR);
	if (fd < 0) {
		perror("/dev/fb0");
		return;
	}
	if (ioctl(fd, FBIOGET_VSCREENINFO, &g_fb_var) < 0) {
		perror("/dev/fb0");
		close(fd);
		return;
	}
	VpostWidth = g_fb_var.xres;
	VpostHeight = g_fb_var.yres;
	VpostBpp = g_fb_var.bits_per_pixel/8;

	g_u32VpostBufMapSize = VpostWidth*VpostHeight*VpostBpp*2;

	g_fb_vaddress = (unsigned int)mmap( NULL, g_u32VpostBufMapSize,
			PROT_READ|PROT_WRITE,
			MAP_SHARED,
			fd,
			0);

	memset ((void*)g_fb_vaddress, 0x0, g_u32VpostBufMapSize );

	if (g_fb_vaddress)
		munmap((void *)g_fb_vaddress, g_u32VpostBufMapSize);

	close(fd);

}
/* ---------------------------------------------------------------------------*/
/**
 * @brief JudgeMonth 判断当月多少天
 *
 * @param year 年份
 * @param month 月份
 *
 * @returns 天数
 */
/* ---------------------------------------------------------------------------*/
int JudgeMonth(int year,int month)
{
	if ((month == 1)
		|| (month == 3)
		|| (month == 5)
		|| (month == 7)
		|| (month == 8)
		|| (month == 10)
		|| (month == 12)) {
		return 31;	//大月
	} else if ((month == 4)
		|| (month == 6)
		|| (month == 9)
		|| (month == 11)) {
		return 30;
	} else if (month == 2) {
		if ((year % 4) == 0) {
			return 28;
		} else {
			return 29;
		}
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief hexToChar  16进制字符串转成n进制字符串
 *
 * @param num 待转换数
 * @param d_str 输出字符串
 * @param radix 进制(1-52)
 */
/* ---------------------------------------------------------------------------*/
void hexToChar( unsigned long long int num, char* d_str, unsigned int radix)
{
	const char a[] = "0123456789ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz";

	char *ptr = d_str;
	if(num == 0) {
		*ptr++ = 0;
		*ptr = '\0';
		return;

	}

	while(num) {
		*ptr++ = a[num % (unsigned long long int)radix];
		num /= (unsigned long long int)radix;

	}

	*ptr = '\0';
	ptr--;
	char *start = d_str;
	while(start < ptr) {
		char tmp = *ptr;
		*ptr = *start;
		*start = tmp;
		ptr--;
		start++;

	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief getDiffSysTick 计算32位差值
 *
 * @param new
 * @param old
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
uint32_t getDiffSysTick(uint64_t new,uint64_t old)
{
    uint32_t diff;
    if (new >= old)
        diff = new - old;
    else
        diff = 0XFFFFFFFF - old + new;
    return diff;
}

static void* getWifiListThread(void *arg)
{
	TcWifiScan *plist = (TcWifiScan *)arg;
	char *cmd[] = { "cat_eye", "wlan0", "scan"  };
	int cnt = 0;
	// 移植iwlist,传入需要的值
	iwlist(3,cmd,plist,&cnt);
	if (wifiloadCallback)
		wifiloadCallback(plist,cnt);
	wifi_scan_end = 1;
	return NULL;
}
int getWifiList(void *ap_info,void (*callback)(void *ap_info,int ap_cnt))
{
	if (wifi_scan_end == 0)
		return 0;
	wifi_scan_end = 0;
	wifiloadCallback = callback;
	return createThread(getWifiListThread,ap_info);
}
int getWifiConfig(int *qual)
{
#ifdef X86
	*qual = 2;
	return 0;
#else
	char *cmd[] = { "cat_eye", "wlan0" };
	return iwconfig(2,cmd,qual);
#endif
}
void wifiConnect(void)
{
	excuteCmd("./check_ip.sh",NULL);
}
