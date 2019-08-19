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

static struct CapType cap;		// 抓拍或录像
static char imei[64 + 1];         // 太川设备机身码
static EtcValueInt etc_private_int[]={
{"cap",			"type",			&cap.type,		0},
{"cap",			"count",		&cap.count,		1},
{"cap",			"timer",		&cap.timer,		5},
};

static EtcValueChar etc_private_char[]={
{"device",		"imei",	    SIZE_CONFIG(imei),		"0"},
};
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
 * @brief configLoadEtcChar 加载char型配置文件
 *
 * @param etc_file 文件数组地址
 * @param length 数组长度
 */
/* ---------------------------------------------------------------------------*/
static void configLoadEtcChar(dictionary *cfg_ini, EtcValueChar *etc_file,
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
	char *sec_private[] = {"cap","device",NULL};

	loadIniFile(&cfg_private_ini,CONFIG_FILENAME,sec_private);
	configLoadEtcInt(cfg_private_ini,etc_private_int,NELEMENTS(etc_private_int));
	configLoadEtcChar(cfg_private_ini,etc_private_char,NELEMENTS(etc_private_char));
	iniparser_freedict(cfg_private_ini);
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
