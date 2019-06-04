#include <sys/ioctl.h>
#include <assert.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "iniparser/iniparser.h"
#include "externfunc.h"
#include "thread_helper.h"
#include "config.h"
#include "debug.h"


#define INI_PUBLIC_FILENAME "config_para.ini"
#define INI_PRIVATE_FILENAME "config.ini"
#define SIZE_CONFIG(x)  x,sizeof(x) - 1

#define NELEMENTS(array)  (sizeof (array) / sizeof ((array) [0]))

typedef enum
{
    CONFIG_CRC,
    CONFIG_SAVE,
    CONFIG_SAVE_PRIVATE,
} ConfigAction;

Config g_config;
static dictionary* cfg_public_ini;
static dictionary* cfg_private_ini;
static pthread_mutex_t cfg_mutex  = PTHREAD_MUTEX_INITIALIZER;

static EtcValueInt etc_public_int[]={
};
static EtcValueChar etc_public_char[]={
};

static EtcValueInt etc_private_int[]={
{"wireless",	"enable",		&g_config.net_config.enable,0},
{"cloud",		"timestamp",	&g_config.timestamp,		0},
};

static EtcValueChar etc_private_char[]={
{"device",		"imei",	    SIZE_CONFIG(g_config.imei),		"0"},
{"device",		"hardcode",	SIZE_CONFIG(g_config.hardcode),	"0"},
{"device",		"version",	SIZE_CONFIG(g_config.version),	DEVICE_SVERSION},
{"cloud",		"app_url",	SIZE_CONFIG(g_config.app_url),	"123"},

{"wireless",	"ssid",	    SIZE_CONFIG(g_config.net_config.ssid),		"MINI"},
{"wireless",	"mode",	    SIZE_CONFIG(g_config.net_config.mode),		"Infra"},
{"wireless",	"security",	SIZE_CONFIG(g_config.net_config.security),	"WPA/WPA2 PSK"},
{"wireless",	"password",	SIZE_CONFIG(g_config.net_config.password),	"12345678"},
{"wireless",	"running",	SIZE_CONFIG(g_config.net_config.running),	"station"},

};


void configSync(void)
{
    sync();
}

static int etcFileCheck(void)
{
	const char * buf = iniparser_getstring(cfg_private_ini, "taichuan:imei", "0");
	if (strcmp(buf,"0") == 0) {
		recoverData(CFG_PRIVATE_DRIVE  INI_PRIVATE_FILENAME);
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

static void SavePublic(void)
{
	configSaveEtcInt(cfg_public_ini,etc_public_int,NELEMENTS(etc_public_int));
	configSaveEtcChar(cfg_public_ini,etc_public_char,NELEMENTS(etc_public_char));
	dumpIniFile(cfg_public_ini,CFG_PUBLIC_DRIVE  INI_PUBLIC_FILENAME);
	DPRINT("[%s]end\n", __FUNCTION__);
}

static void SavePrivate(void)
{
	configSaveEtcInt(cfg_private_ini,etc_private_int,NELEMENTS(etc_private_int));
	configSaveEtcChar(cfg_private_ini,etc_private_char,NELEMENTS(etc_private_char));
	dumpIniFile(cfg_private_ini,CFG_PRIVATE_DRIVE  INI_PRIVATE_FILENAME);
	if (etcFileCheck() == 1) {
		backData(CFG_PRIVATE_DRIVE  INI_PRIVATE_FILENAME);
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

void configLoad(void)
{
	char *sec_private[] = {"device","wireless","cloud",NULL};

	int ret = loadIniFile(&cfg_private_ini,CFG_PRIVATE_DRIVE  INI_PRIVATE_FILENAME,sec_private);
	configLoadEtcInt(cfg_private_ini,etc_private_int,NELEMENTS(etc_private_int));
	configLoadEtcChar(cfg_private_ini,etc_private_char,NELEMENTS(etc_private_char));
	etcFileCheck();
	if (ret || strcmp(g_config.version,DEVICE_SVERSION) != 0) {
		strcpy(g_config.version,DEVICE_SVERSION);
		SavePrivate();
	}
	printf("imei:%s,hard:%s\n", g_config.imei,g_config.hardcode);
	// 判断是否APP地址图片
	if (fileexists(QRCODE_APP) == 0) {
		
	}
}


static void* ConfigSaveTask(void* arg)
{
    int* args = (int*) arg;
    ConfigAction action = (ConfigAction) args[0];
	int func = 0;

    pthread_mutex_lock(&cfg_mutex);

	if (action == CONFIG_SAVE) {
		func = (int)args[2];
        SavePublic();
	} else if (action == CONFIG_SAVE_PRIVATE)  {
		func = (int)args[2];
		SavePrivate();
	}

#ifdef CFG_CHECK_FILES_CRC_ON_BOOTING
    char* filepath = (char*) args[1];
    if (action != CONFIG_SAVE_PRIVATE)
        UpgradeSetFileCrc(filepath);
#endif
	configSync();
	if(func) {
		configCallback p = (configCallback)func;
		p();
	}

    pthread_mutex_unlock(&cfg_mutex);

    return NULL;
}

void ConfigUpdateCrc(char* filepath)
{
#ifdef CFG_CHECK_FILES_CRC_ON_BOOTING
    static int args[2];
    static char path[256];

    strcpy(path, filepath);

    args[0] = CONFIG_CRC;
    args[1] = (int)path;

    createThread(ConfigSaveTask, args);
#endif // CFG_CHECK_FILES_CRC_ON_BOOTING
}

void ConfigSave(configCallback func)
{
    static int args[3];

    args[0] = CONFIG_SAVE;
    args[1] = (int)CFG_PUBLIC_DRIVE  INI_PUBLIC_FILENAME;
	args[2] = (int)func;

    createThread(ConfigSaveTask, args);
}

void ConfigSavePrivate(void)
{
    static int args[3];

    args[0] = CONFIG_SAVE_PRIVATE;
    args[1] = (int)CFG_PRIVATE_DRIVE  INI_PRIVATE_FILENAME;

    createThread(ConfigSaveTask, args);
}

void ConfigSavePrivateCallback(configCallback func)
{
    static int args[3];

    args[0] = CONFIG_SAVE_PRIVATE;
    args[1] = (int)CFG_PRIVATE_DRIVE  INI_PRIVATE_FILENAME;
	args[2] = (int)func;

    createThread(ConfigSaveTask, args);
}

