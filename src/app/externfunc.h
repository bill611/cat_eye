#ifndef EXTERNFUNC_H
#define EXTERNFUNC_H

#include <stdint.h>
typedef struct _st_dir {
	char 	path[128];		// 文件名称 包括路径
	void *	data;			// 文件
}ST_DIR;

#define SaveLog(fmt,arg...)   \
	do { \
		char RecDate[50]; \
		FILE *log_fd; \
		getDate(RecDate,sizeof(RecDate)); \
		log_fd = fopen("log.txt","a+"); \
		fprintf(log_fd,"%s:[%s]",RecDate,__FUNCTION__); \
		fprintf(log_fd,fmt,##arg); \
		fflush(log_fd); \
		fclose(log_fd); \
	} while(0) \
//---------------------------------------------------------------------------
//外部函数
//---------------------------------------------------------------------------
int fileexists(const char *FileName);
char *strupper(char *pdst,const char *pstr,int Size);
void DelayMs(int ms);
char * excuteCmd(char *Cmd,...);
void ErrorLog(int ecode,const char *fmt,...);
char * getDate(char *cBuf,int Size);
const char * GetSendIP(const char *pSrcIP,const char *pDestIP,const char *pMask);
int jugdeRecIP(const char *pSrcIP,const char *pDestIP,const char *pMask);
unsigned int my_inet_addr(const char *IP);

void SetNetwork(int flag,const char *cIp,const char *cMask,const char *cGateWay,const char *cMac);
void SetNetMac(unsigned char *pImei,char *MAC);
uint64_t getMs(void);
void backData(char *file);
unsigned long long GetFlashFreeSpace(void);
unsigned long long GetFlashFreeSpaceKB(void);
unsigned long long GetFlashFreeSpaceMB(void);

void print_data(char *data,int len);
int GetFilesNum(char *pPathDir,void (*func)(void *));

int recoverData(const char *file);
struct tm * getTime(void);
int getWifiList(void *ap_info,void (*callback)(void *ap_info,int ap_cnt));
int getWifiConfig(int *qual);
void wifiConnectStart(void);
void wifiConnect(void);
void wifiDisConnect(void);
int screensaverSet(int state);
void getFileName(char *file_name,char *date);
void getCpuId(char *hardcode);
uint64_t MyGetTickCount(void);
int GetFileSize(char *file);
int getLocalIP(char *IP,char *gateway);
int getGateWayMac(char *gateway,char *mac);
void powerOff(void);
int checkSD(void);
int getSdMem(char *total,char *residue,char *used);
int adjustdate(int year,int mon,int mday,int hour,int min,int sec);
#endif

