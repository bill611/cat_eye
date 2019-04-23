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
};

static EtcValueChar etc_private_char[]={
{"taichuan",	"imei",	            SIZE_CONFIG(g_config.imei),		"0"},
{"taichuan",	"version",	        SIZE_CONFIG(g_config.version),		CATEYE_VERSION},

{"ethernet",	"dhcp",	    SIZE_CONFIG(g_config.net_config.dhcp),		"1"},
{"ethernet",	"ipaddr",	SIZE_CONFIG(g_config.net_config.ipaddr),	"172.16.10.10"},
{"ethernet",	"netmask",	SIZE_CONFIG(g_config.net_config.netmask),	"255.255.0.0"},
{"ethernet",	"gateway",	SIZE_CONFIG(g_config.net_config.gateway),	"172.16.1.1"},
{"ethernet",	"macaddr",	SIZE_CONFIG(g_config.net_config.macaddr),	"00:01:02:03:04:05"},
{"ethernet",	"firstdns",	SIZE_CONFIG(g_config.net_config.firstdns),	"114.114.114.114"},
{"ethernet",	"backdns",	SIZE_CONFIG(g_config.net_config.backdns),	"8.8.8.8"},

{"wireless",	"ssid",	    SIZE_CONFIG(g_config.net_config.ssid),		"MINI"},
{"wireless",	"mode",	    SIZE_CONFIG(g_config.net_config.mode),		"Infra"},
{"wireless",	"security",	SIZE_CONFIG(g_config.net_config.security),	"WPA/WPA2 PSK"},
{"wireless",	"password",	SIZE_CONFIG(g_config.net_config.password),	"12345678"},
{"wireless",	"running",	SIZE_CONFIG(g_config.net_config.running),	"station"},

{"softap",	    "s_ssid",	 SIZE_CONFIG(g_config.net_config.s_ssid),	  "alitest"},
{"softap",	    "s_password",SIZE_CONFIG(g_config.net_config.s_password),"12345678"},
};


void configSync(void)
{
#if defined(CFG_NOR_ENABLE) && CFG_NOR_CACHE_SIZE > 0
    ioctl(ITP_DEVICE_NOR, ITP_IOCTL_FLUSH, NULL);
#endif
}
static int etcFileCheck(void)
{
	const char * buf = iniparser_getstring(cfg_private_ini, "gateway:product_key", "0");
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
        // DPRINT("[%s]%s,%d\n", __FUNCTION__,buf,*etc_file->value);
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
	char *sec_private[] = {"gateway","taichuan","ethernet","wireless","softap",NULL};

	int ret = loadIniFile(&cfg_private_ini,CFG_PRIVATE_DRIVE  INI_PRIVATE_FILENAME,sec_private);
	configLoadEtcInt(cfg_private_ini,etc_private_int,NELEMENTS(etc_private_int));
	configLoadEtcChar(cfg_private_ini,etc_private_char,NELEMENTS(etc_private_char));
	etcFileCheck();
	if (ret || strcmp(g_config.version,CATEYE_VERSION) != 0)
		strcpy(g_config.version,CATEYE_VERSION);
		SavePrivate();
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
    // sendCmdConfigSave(func);

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

void ConfigSave(void (*func)(void))
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

void ConfigSavePrivateCallback(void (*func)(void))
{
    static int args[3];

    args[0] = CONFIG_SAVE_PRIVATE;
    args[1] = (int)CFG_PRIVATE_DRIVE  INI_PRIVATE_FILENAME;
	args[2] = (int)func;

    createThread(ConfigSaveTask, args);
}

