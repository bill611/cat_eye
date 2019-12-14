/*
 * =============================================================================
 *
 *       Filename:  my_ntp.c
 *
 *    Description:  同步时间
 *
 *        Version:  1.0
 *        Created:  2019-05-21 16:45:16
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
#include <string.h>
#include <unistd.h>
#include <netdb.h>
#include <time.h>
#include <stdarg.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/types.h>
#include <arpa/inet.h>
#include <sys/select.h>
#include <sys/param.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/time.h>
#include "my_ntp.h"
#include "externfunc.h"
#include "thread_helper.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
//授时服务器 端口默认 123
#define DEF_NTP_SERVER "ntp.neu.edu.cn" //东北大学网络授时服务,为您提供高精度的网络授时服务

//#define DEF_NTP_SERVER_IP "202.112.31.197"	//原始
#define DEF_NTP_SERVER_IP "120.24.152.147"		//测试使用
#define DEF_NTP_PORT 123

//默认请求数据包填充
#define LI 0   //协议头中的元素
#define VN 3   //版本
#define MODE 3 //模式 : 客户端请求
#define STRATUM 0
#define POLL 4  //连续信息间的最大间隔
#define PREC -6 //本地时钟精度

//校验时间计算用到的宏

//ntp时间从年开始，本地时间从年开始，这是两者之间的差值
#define JAN_1970 0x83aa7e80 //3600s*24h*(365days*70years+17days)
//x*10^(-6)*2^32 微妙数转 NtpTime 结构的 fraction 部分
#define NTPFRAC(x) (4294 * (x) + ((1981 * (x)) >> 11))
//NTPFRAC的逆运算
#define USEC(x) (((x) >> 12) - 759 * ((((x) >> 10) + 32768) >> 16))

#define MKSEC(ntpt) ((ntpt).integer - JAN_1970)
#define MKUSEC(ntpt) (USEC((ntpt).fraction))
#define TTLUSEC(sec, usec) ((long long)(sec)*1000000 + (usec))
#define GETSEC(us) ((us) / 1000000)
#define GETUSEC(us) ((us) % 1000000)

#define DATA(i) ntohl(((unsigned int *)data)[i])

#define PDEBUG(fmt, args...) printf("[%s:%d]" fmt "\n", __func__, __LINE__, ##args)

//ntp时间戳结构
typedef struct
{
    unsigned int integer;
    unsigned int fraction;
} NtpTime;


/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static void (*getTimeCallBcak)(void);
static int need_update = 0; // 是否需要同步网络时间
static long update_period = 0; // 更新周期，减为0时立即更新
static pthread_mutex_t mutex ;
static void ntpSendPacket(int fd)
{
    unsigned int data[12];
    int ret;
    struct timeval now;
    if (sizeof(data) != 48) {
        PDEBUG("data 长度小于48!");
        return;
    }
    memset((char *)data, 0, sizeof(data));
    data[0] = htonl((LI << 30) | (VN << 27) | (MODE << 24) | (STRATUM << 16) | (POLL << 8) | (PREC & 0xff));
    data[1] = htonl(1 << 16);
    data[2] = htonl(1 << 16);
    //获得本地时间
    gettimeofday(&now, NULL);

    data[10] = htonl(now.tv_sec + JAN_1970);
    data[11] = htonl(NTPFRAC(now.tv_usec));

    ret = send(fd, data, 48, 0);
}

static int ntpGetServerTime(int sock, struct timeval *newtime)
{
    int ret;
    unsigned int data[12];
    NtpTime oritime, rectime, tratime, destime;
    struct timeval offtime, dlytime;
    struct timeval now;

    bzero(data, sizeof(data));
    ret = recvfrom(sock, data, sizeof(data), 0, NULL, 0);
    if (ret == -1) {
        PDEBUG("读取返回数据失败\n");
        return 1;
    } else if (ret == 0) {
        PDEBUG("读取到速度长度: 0!\n");
        return 1;
    }

    //1970逆转换到1900
    gettimeofday(&now, NULL);
    destime.integer = now.tv_sec + JAN_1970;
    destime.fraction = NTPFRAC(now.tv_usec);

    //字节序转换
    oritime.integer = DATA(6);
    oritime.fraction = DATA(7);
    rectime.integer = DATA(8);
    rectime.fraction = DATA(9);
    tratime.integer = DATA(10);
    tratime.fraction = DATA(11);

    //Originate Timestamp       T1        客户端发送请求的时间
    //Receive Timestamp        T2        服务器接收请求的时间
    //Transmit Timestamp       T3        服务器答复时间
    //Destination Timestamp     T4        客户端接收答复的时间
    //网络延时 d 和服务器与客户端的时差 t
    //d = (T2 - T1) + (T4 - T3); t = [(T2 - T1) + (T3 - T4)] / 2;

    long long orius, recus, traus, desus, offus, dlyus;

    orius = TTLUSEC(MKSEC(oritime), MKUSEC(oritime));
    recus = TTLUSEC(MKSEC(rectime), MKUSEC(rectime));
    traus = TTLUSEC(MKSEC(tratime), MKUSEC(tratime));
    desus = TTLUSEC(now.tv_sec, now.tv_usec);

    offus = ((recus - orius) + (traus - desus)) / 2;
    dlyus = (recus - orius) + (desus - traus);

    offtime.tv_sec = GETSEC(offus);
    offtime.tv_usec = GETUSEC(offus);
    dlytime.tv_sec = GETSEC(dlyus);
    dlytime.tv_usec = GETUSEC(dlyus);

    struct timeval new;

    //粗略校时
    //new.tv_sec = tratime.integer - JAN_1970;
    //new.tv_usec = USEC(tratime.fraction);
    //精确校时
    new.tv_sec = destime.integer - JAN_1970 + offtime.tv_sec;
    new.tv_usec = USEC(destime.fraction) + offtime.tv_usec;

    //提取现在好的时间
    *newtime = new;
    return 0;
}

/*
 * 更新本地时间
 * @newtime -- 要新的时间
 * */
static int ntpModLocalTime(struct timeval newtime)
{
	struct tm *tm_p;
	time_t time_sec = newtime.tv_sec + 28800;
	//time_t time_sec = newtime.tv_sec;
    tm_p = gmtime(&time_sec);

#ifndef X86
	adjustdate(tm_p->tm_year + 1900,
			tm_p->tm_mon + 1,
		   	tm_p->tm_mday,
		   	tm_p->tm_hour,
		   	tm_p->tm_min,
		   	tm_p->tm_sec);
    // system(time_buff);
#endif
	if (getTimeCallBcak)
		getTimeCallBcak();
    return 0;
}


static void *ntpTimeThread(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
    int ret;
    int sock;
    struct timeval newtime;
    struct timeval timeout;

    int addr_len = sizeof(struct sockaddr_in);
    struct sockaddr_in addr_src; //本地 socket  <netinet/in.h>
    struct sockaddr_in addr_dst; //服务器 socket

    //UDP数据报套接字
    sock = socket(PF_INET, SOCK_DGRAM, 0);
    if (sock == -1) {
        PDEBUG("套接字创建失败，被迫终止 ! \n");
		return NULL;
    }

    memset(&addr_src, 0, addr_len);
    addr_src.sin_family = AF_INET;
    addr_src.sin_port = htons(0);
    addr_src.sin_addr.s_addr = htonl(INADDR_ANY); //<arpa/inet.h>
    //绑定本地地址
    if (-1 == bind(sock, (struct sockaddr *)&addr_src, addr_len))
    {
        PDEBUG("绑定失败，被迫终止 !\n");
		goto end_thread;
    }
    memset(&addr_dst, 0, addr_len);
    addr_dst.sin_family = AF_INET;
    addr_dst.sin_port = htons(DEF_NTP_PORT);
	addr_dst.sin_addr.s_addr = inet_addr((char *)arg);


    if (-1 == connect(sock, (struct sockaddr *)&addr_dst, addr_len)) {
        PDEBUG("连接服务器失败，被迫终止 !\n");
		goto end_thread;
    }

	do {
		if (!need_update) {
			sleep(1);
			continue;
		}
		if (update_period) {
			pthread_mutex_lock(&mutex);
			update_period--;
			pthread_mutex_unlock(&mutex);
			if (update_period > 0) {
				sleep(1);
				continue;
			}
		}
		//发送 ntp 包
		ntpSendPacket(sock);
        fd_set fds_read;
        FD_ZERO(&fds_read);
        FD_SET(sock, &fds_read);

        timeout.tv_sec = 30;
        timeout.tv_usec = 0;

        ret = select(sock + 1, &fds_read, NULL, NULL, &timeout);
        if (ret == -1) {
            PDEBUG("select函数出错，被迫终止 !\n");
			break;
        }
        if (ret == 0) {
            PDEBUG("等待服务器响应超时，重发请求 !\n");
            sleep(1);
            continue;
        }
        if (FD_ISSET(sock, &fds_read)) {
            if (1 == ntpGetServerTime(sock, &newtime))
                continue;
            ntpModLocalTime(newtime);
			pthread_mutex_lock(&mutex);
			update_period = 60*60*24;
			pthread_mutex_unlock(&mutex);
        }
	} while (1);

end_thread:
    close(sock);
    return NULL;
}

void ntpTime(char *server_ip,void (*callBack)(void))
{
	getTimeCallBcak = callBack;

	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&mutex, &mutexattr);
    pthread_mutexattr_destroy(&mutexattr);

	if (server_ip)
		createThread(ntpTimeThread,server_ip);
}

void ntpEnable(int enable)
{
	need_update = enable;
    pthread_mutex_lock(&mutex);
	update_period = 0;
    pthread_mutex_unlock(&mutex);

}
