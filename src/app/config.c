#include <sys/ioctl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <errno.h>
#include "externfunc.h"
#include "thread_helper.h"
#include "config.h"
#include "debug.h"


#define SIZE_CONFIG(x)  x,sizeof(x) - 1

#define NELEMENTS(array)  (sizeof (array) / sizeof ((array) [0]))

Config g_config;
struct SaveType {
	int cmd; // 0 private,  1public
	configCallback func; // 回调函数
};
static dictionary* cfg_public_ini;
static dictionary* cfg_private_ini;
static pthread_mutex_t cfg_mutex ;

static EtcValueInt etc_public_int[]={
{"cloud",		"timestamp",	&g_config.timestamp,		0},
{"wireless",	"enable",		&g_config.net_config.enable,0},
{"cap_doorbell","type",			&g_config.cap_doorbell.type,	0},
{"cap_doorbell","count",		&g_config.cap_doorbell.count,	1},
{"cap_doorbell","timer",		&g_config.cap_doorbell.timer,	5},
{"cap_alarm",	"type",			&g_config.cap_alarm.type,	0},
{"cap_alarm",	"count",		&g_config.cap_alarm.count,	1},
{"cap_alarm",	"timer",		&g_config.cap_alarm.timer,	5},
{"cap_talk",	"type",			&g_config.cap_talk.type,	0},
{"cap_talk",	"count",		&g_config.cap_talk.count,	1},
{"cap_talk",	"timer",		&g_config.cap_talk.timer,	10},
{"rings",		"ring_num",		&g_config.ring_num,			0},
{"rings",		"ring_volume",	&g_config.ring_volume,		80},
{"rings",		"alarm_volume",	&g_config.alarm_volume,		80},
{"rings",		"talk_volume",	&g_config.talk_volume,		80},
{"others",		"pir_active_times",	&g_config.pir_active_times,		20},
{"others",		"pir_alarm",	&g_config.pir_alarm,		0},
{"others",		"pir_strength",	&g_config.pir_strength,		1},
{"others",		"screensaver_time",	&g_config.screensaver_time,		11},
{"others",		"brightness",	&g_config.brightness,		80},
{"others",		"record_time",	&g_config.record_time,		30},
};
static EtcValueChar etc_public_char[]={
{"device",		"version",	SIZE_CONFIG(g_config.version),	DEVICE_SVERSION},
{"device",		"s_version",SIZE_CONFIG(g_config.s_version),"0"},
{"cloud",		"app_url",	SIZE_CONFIG(g_config.app_url),	"123"},
{"wireless",	"ssid",	    SIZE_CONFIG(g_config.net_config.ssid),		"MINI"},
{"wireless",	"mode",	    SIZE_CONFIG(g_config.net_config.mode),		"Infra"},
{"wireless",	"security",	SIZE_CONFIG(g_config.net_config.security),	"WPA/WPA2 PSK"},
{"wireless",	"password",	SIZE_CONFIG(g_config.net_config.password),	"12345678"},
{"wireless",	"running",	SIZE_CONFIG(g_config.net_config.running),	"station"},
};

static EtcValueChar etc_private_char[]={
{"device",		"imei",	    SIZE_CONFIG(g_config.imei),		"0"},
{"face",		"license",	SIZE_CONFIG(g_config.f_license),"0"},

};
static char *sec_private[] = {
	"device",
	"face",
	NULL
};

static char *sec_public[] = {
	"device",
	"cloud",
	"wireless",
	"rings",
	"cap_doorbell",
	"cap_alarm",
	"cap_talk",
	"others",
	NULL
};

void configSync(void)
{
    sync();
}

static int etcFileCheck(void)
{
	const char * buf = iniparser_getstring(cfg_private_ini, "taichuan:imei", "0");
	if (strcmp(buf,"0") == 0) {
		recoverData(CONFIG_FILENAME);
		return 0;
	} else {
		return 1;
	}
}

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
		printf("[%s]%s,%d\n", __FUNCTION__,buf,*etc_file->value);
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
void configLoadEtcChar(dictionary *cfg_ini, EtcValueChar *etc_file,
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
 * @brief configSaveEtcInt 加载int型配置文件
 *
 * @param etc_file 文件数组地址
 * @param length 数组长度
 */
/* ---------------------------------------------------------------------------*/
static void configSaveEtcInt(dictionary *cfg_ini, EtcValueInt *etc_file,
		unsigned int length)
{
	unsigned int i;
	char buf[64];
	char data[64];
	for (i=0; i<length; i++) {
		sprintf(buf,"%s:%s",etc_file->section,etc_file->key);
		sprintf(data,"%d",*etc_file->value);
		iniparser_set(cfg_ini, buf, data);
		etc_file++;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief configSaveEtcChar 加载char型配置文件
 *
 * @param etc_file 文件数组地址
 * @param length 数组长度
 */
/* ---------------------------------------------------------------------------*/
static void configSaveEtcChar(dictionary *cfg_ini, EtcValueChar *etc_file,
		unsigned int length)
{
	unsigned int i;
	char buf[64];
	for (i=0; i<length; i++) {
		sprintf(buf,"%s:%s",etc_file->section,etc_file->key);
		// DPRINT("[%d]iniparser_set:%s--%s\n",i,buf,etc_file->value );
		iniparser_set(cfg_ini, buf, etc_file->value);
		etc_file++;
	}
}

/* ---------------------------------------------------------------------------*/
/**
 * @brief dumpIniFile iniparser_dump_ini 配置
 *
 * @param d
 * @param file_name
 */
/* ---------------------------------------------------------------------------*/
static void dumpIniFile(dictionary *d,char *file_name)
{
    FILE* f;
    // save to file
    f = fopen(file_name, "wb");
	if (!f) {
	    DPRINT("cannot open ini file: %s\n", file_name);
        return;
    }

    iniparser_dump_ini(d, f);
	fflush(f);
    fclose(f);

}

static void SavePrivate(void)
{
	configSaveEtcChar(cfg_private_ini,etc_private_char,NELEMENTS(etc_private_char));
	dumpIniFile(cfg_private_ini,CONFIG_FILENAME);
	if (etcFileCheck() == 1) {
		backData(CONFIG_FILENAME);
	}
	DPRINT("[%s]end\n", __FUNCTION__);
}

static void SavePublic(void)
{
	configSaveEtcInt(cfg_public_ini,etc_public_int,NELEMENTS(etc_public_int));
	configSaveEtcChar(cfg_public_ini,etc_public_char,NELEMENTS(etc_public_char));
	dumpIniFile(cfg_public_ini,CONFIG_PARA_FILENAME);
	if (etcFileCheck() == 1) {
		backData(CONFIG_PARA_FILENAME);
	}
	DPRINT("[%s]end\n", __FUNCTION__);
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

void createSdcardDirs(void)
{
	if (checkSD() == -1)
		return;
	mkdir(CAP_PATH, 		S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	mkdir(TALK_PATH, 		S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	mkdir(ALARM_PATH, 		S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
	mkdir(FACE_PATH, 		S_IRWXU | S_IRWXG | S_IROTH | S_IXOTH);
}
void configLoad(void)
{
	pthread_mutexattr_t mutexattr;
	pthread_mutexattr_init(&mutexattr);
    pthread_mutexattr_settype(&mutexattr, PTHREAD_MUTEX_RECURSIVE_NP);
    pthread_mutex_init(&cfg_mutex, &mutexattr);
    pthread_mutexattr_destroy(&mutexattr);

	int ret = loadIniFile(&cfg_private_ini,CONFIG_FILENAME,sec_private);
	configLoadEtcChar(cfg_private_ini,etc_private_char,NELEMENTS(etc_private_char));
	etcFileCheck();
	if (ret) 
		SavePrivate();
	ret = loadIniFile(&cfg_public_ini,CONFIG_PARA_FILENAME,sec_public);
	configLoadEtcChar(cfg_public_ini,etc_public_char,NELEMENTS(etc_public_char));
	configLoadEtcInt(cfg_public_ini,etc_public_int,NELEMENTS(etc_public_int));
	
	if (ret || strcmp(g_config.version,DEVICE_SVERSION) != 0) {
		strcpy(g_config.version,DEVICE_SVERSION);
		SavePublic();
	}
	getCpuId(g_config.hardcode);
	printf("imei:%s,hard:%s\n", g_config.imei,g_config.hardcode);
#if 0
	// 手动生成二维码图片
	char buf[128] = {0};
	sprintf(buf,"%s/%s",g_config.imei,g_config.hardcode);
	qrcodeString(buf,QRCODE_IMIE);
#endif
	createSdcardDirs();
}


static void* ConfigSaveTask(void* arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
    pthread_mutex_lock(&cfg_mutex);
	struct SaveType *save_type = (struct SaveType *)arg;

	if (save_type->cmd == 0)
		SavePrivate();
	else if (save_type->cmd == 1)
		SavePublic();

	configSync();
	if(save_type->func)
		save_type->func();

    pthread_mutex_unlock(&cfg_mutex);

    return NULL;
}


void ConfigSavePrivate(void)
{
	static struct SaveType save_type = { 0,NULL };
    createThread(ConfigSaveTask, &save_type);
}

void ConfigSavePrivateCallback(configCallback func)
{
	static struct SaveType save_type = { 0, NULL };
	save_type.func = func;
    createThread(ConfigSaveTask, &save_type);
}

void ConfigSavePublic(void)
{
	static struct SaveType save_type = { 1,NULL };
    createThread(ConfigSaveTask, &save_type);
}

void ConfigSavePublicCallback(configCallback func)
{
	static struct SaveType save_type = { 1, NULL };
	save_type.func = func;
    createThread(ConfigSaveTask, &save_type);
}

