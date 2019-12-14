#include <sys/ioctl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "config.h"
#include "iniparser/iniparser.h"

#define DPRINT(...)           \
do {                          \
    printf("\033[1;34m");  \
    printf("[UART->%s,%d]",__func__,__LINE__);   \
    printf(__VA_ARGS__);      \
    printf("\033[0m");        \
} while (0)


#define SIZE_CONFIG(x)  x,sizeof(x) - 1

#define NELEMENTS(array)  (sizeof (array) / sizeof ((array) [0]))

static dictionary* cfg_private_ini = NULL;
static dictionary* cfg_public_ini = NULL;

static void configLoadEtcInt(dictionary *cfg_ini, EtcValueInt *etc_file,
		unsigned int length);

static int brightness = 0;
static struct CapType cap;		// 抓拍或录像
static char imei[64 + 1];         // 太川设备机身码
static struct Mute mute;
static EtcValueInt etc_public_int[]={
{"cap_doorbell","type",		&cap.type,		0},
{"cap_doorbell","count",	&cap.count,		1},
{"cap_doorbell","timer",	&cap.timer,		5},
{"others",		"brightness",	&brightness,		80},
{"rings",		"mute_state",		&mute.state,		0},
{"rings",		"mute_start_time",	&mute.start_time,	0},
{"rings",		"mute_end_time",	&mute.end_time,	1439},
};

static EtcValueChar etc_private_char[]={
{"device",		"imei",	    SIZE_CONFIG(imei),		"0"},
};
static char *sec_private[] = {"device",NULL};
static char *sec_public[] = {"cap_doorbell","others","rings",NULL};
/* ---------------------------------------------------------------------------*/
/**
 * @brief configoadEtcInt 加载int型配置文件
 *
 * @param etc_file 文件数组地址
 * @param length 数组长度
 */
/* ---------------------------------------------------------------------------*/
static void configLoadEtcInt(dictionary *cfg_ini, EtcValueInt *etc_file,
		unsigned int length)
{
	unsigned int i;
	char buf[64];
	for (i=0; i<length; i++) {
		sprintf(buf,"%s:%s",etc_file->section,etc_file->key);
		*etc_file->value = iniparser_getint(cfg_ini, buf, etc_file->default_int);
		DPRINT("%s,%d\n", buf,*etc_file->value);
		etc_file++;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief sconfigLoadEtcChar 加载char型配置文件
 *
 * @param etc_file 文件数组地址
 * @param length 数组长度
 */
/* ---------------------------------------------------------------------------*/
static void sconfigLoadEtcChar(dictionary *cfg_ini, EtcValueChar *etc_file,
		unsigned int length)
{
	unsigned int i;
	char buf[64];
	for (i=0; i<length; i++) {
		sprintf(buf,"%s:%s",etc_file->section,etc_file->key);
		strncpy(etc_file->value,
			   	iniparser_getstring(cfg_ini, buf, etc_file->default_char),
			   	etc_file->leng);
		// DPRINT("[%s]%s,%s\n", __FUNCTION__,buf,etc_file->value);
		etc_file++;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief loadIniFile 加载ini文件，同时检测字段完整性
 *
 * @param d
 * @param file_path
 * @param sec[]
 *
 * @returns >0 有缺少字段，需要保存更新， 0无缺少字段，正常
 */
/* ---------------------------------------------------------------------------*/
static int loadIniFile(dictionary **d,char *file_path,char *sec[])
{
	int ret = 0;
	int i;
    *d = iniparser_load(file_path);
    if (*d == NULL) {
        *d = dictionary_new(0);
        assert(*d);
		ret++;
		for (i=0; sec[i] != NULL; i++)
			iniparser_set(*d, sec[i], NULL);
	} else {
		int nsec = iniparser_getnsec(*d);
		int j;
		const char *  secname;
		for (i=0; sec[i] != NULL; i++) {
			for (j=0; j<nsec; j++) {
				secname = iniparser_getsecname(*d, j);
				if (strcasecmp(secname,sec[i]) == 0)
					break;
			}
			if (j == nsec)  {
				ret++;
				iniparser_set(*d, sec[i], NULL);
			}
		}
	}
	return ret;
}

void sconfigLoad(void)
{
	loadIniFile(&cfg_private_ini,CONFIG_FILENAME,sec_private);
	sconfigLoadEtcChar(cfg_private_ini,etc_private_char,NELEMENTS(etc_private_char));
	iniparser_freedict(cfg_private_ini);

	loadIniFile(&cfg_public_ini,CONFIG_PARA_FILENAME,sec_public);
	configLoadEtcInt(cfg_public_ini,etc_public_int,NELEMENTS(etc_public_int));
	iniparser_freedict(cfg_public_ini);
}

int getCapType(void)
{
	return cap.type;
}

int getCapCount(void)
{
	return cap.count;
}

int getCapTimer(void)
{
	return cap.timer;
}
char * getCapImei(void)
{
	return imei;
}
int getBrightness(void)
{
	return brightness;
}
int sIsNeedToPlay(void)
{
	if (mute.state == 0)		
		return 1;
	time_t timer;
	timer = time(&timer);
	struct tm *tm = localtime(&timer);
	int time_now = tm->tm_hour * 60 + tm->tm_min;
	if (mute.start_time <= mute.end_time) {
		if (time_now >= mute.start_time && time_now <= mute.end_time)
			return 0;
		else 
			return 1;
	} else {
		if (time_now <= mute.end_time)	
			return 1;
		if (time_now >= mute.start_time)
			return 1;
		return 0;
	}
}
