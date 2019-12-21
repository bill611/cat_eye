/*
 * =============================================================================
 *
 *       Filename:  my_update.c
 *
 *    Description:  升级流程
 *
 *        Version:  1.0
 *        Created:  2019-09-03 14:19:57
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
#include <string.h>
#include <assert.h>
#include <stdlib.h>
#include <unistd.h>
#include "iniparser/iniparser.h"
#include "thread_helper.h"
#include "externfunc.h"
#include "my_update.h"
#include "my_http.h"
#include "remotefile.h"
#include "config.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#define SIZE_CONFIG(x)  x,sizeof(x) - 1
#define NELEMENTS(array)  (sizeof (array) / sizeof ((array) [0]))

struct VersionType{
	int major;   // 主版本，区分设备类型及方案平台
	int minor;   // 次版本，有功能更新时增加
	int release; // 发布版本，修改bug时增加
};
typedef struct _UpdateFileInfo {
	char dev_type[64];         // 设备类型
	char version[16];      // 升级包软件版本
	char start_version[16];      // 升级包升级软件起始版本
	char end_version[16];      // 升级包升级软件结束版本
	char force[8];      // 是否强制升级，0否 1是
	char url[128];     // 升级包地址 
	char readme[256];     // 升级内容
	int need_update;     // 是否需要升级，0不需要，1需要，强制升级时，直接进入升级界面
} UpdateFileInfo;

struct _MyUpdatePrivate {
	char ip[16];
	char file_path[512];
	int port;
	int type;
	int check_update;
	int is_updating;
	TRemoteFile * remote;
	UpdateFunc callbackFunc;
};

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
MyUpdate *my_update = NULL;
static MyHttp *http = NULL;
static UpdateFileInfo update_info;
static dictionary* update_ini = NULL;

static EtcValueChar etc_update_char[]={
{"VersionInfo",		"DevType",	    SIZE_CONFIG(update_info.dev_type),		"0"},
{"VersionInfo",		"Version",	    SIZE_CONFIG(update_info.version),		"0"},
{"VersionInfo",		"StartVersion",	SIZE_CONFIG(update_info.start_version),	"0"},
{"VersionInfo",		"EndVersion",	SIZE_CONFIG(update_info.end_version),	"0"},
{"VersionInfo",		"Force",	    SIZE_CONFIG(update_info.force),			"0"},
{"VersionInfo",		"Url",	    	SIZE_CONFIG(update_info.url),			"0"},
{"VersionInfo",		"Readme",	    SIZE_CONFIG(update_info.readme),		"0"},
};


/* ---------------------------------------------------------------------------*/
/**
 * @brief tarUpdateFiles 
 * 解压升级包
 * 判断升级包完整新
 * 执行升级
 * 退出程序重启
 */
/* ---------------------------------------------------------------------------*/
static void tarUpdateFiles(int type,char *path)
{
	char update_buf[64] = {0};
    char update_go_buf[64] = {0};
	sprintf(update_buf,"tar xzf %supdate.tar.gz -C %s",path,path);
	int ret = system(update_buf);
    sprintf(update_go_buf,"%supdate/go.sh",path);
	if (ret == 0 && fileexists(update_go_buf)) {
        if (type == UPDATE_TYPE_SDCARD){
            if (my_update->interface->uiUpdateSdCard)	
                my_update->interface->uiUpdateSdCard();
            sleep(1);
            exit(1);
        } else {
            system(update_go_buf);
            if (my_update->interface->uiUpdateSuccess)	
                my_update->interface->uiUpdateSuccess();
            sync();
            sleep(1);
            exit(0);
        }
	} else {
		if (my_update->interface->uiUpdateFail)
			my_update->interface->uiUpdateFail();
	}
}
static int init(struct _MyUpdate * This,int type,char *ip,int port,char *file_path,UpdateFunc callbackFunc)
{
	This->priv->type = type;	
	This->priv->callbackFunc = callbackFunc;	
	if (This->priv->type == UPDATE_TYPE_CENTER) {
		This->priv->port = port;	
		if (ip)
			strcpy(This->priv->ip,ip);
		if (file_path)
			strcpy(This->priv->file_path,file_path);
		This->priv->remote =  CreateRemoteFile(ip,"Update1.cab");
	}
	return 0;
}
static int download(struct _MyUpdate *This)
{
	if (This->priv->type == UPDATE_TYPE_CENTER) {
		This->priv->remote->Download(This->priv->remote,
				0,This->priv->file_path,UPDATE_INI_PATH"update.tar.gz",FALSE,This->priv->callbackFunc);
		tarUpdateFiles(This->priv->type,UPDATE_INI_PATH);
		
	} else if (This->priv->type == UPDATE_TYPE_SDCARD) {
        tarUpdateFiles(This->priv->type,SDCARD_PATH);
	} else if (This->priv->type == UPDATE_TYPE_SERVER) {
		int leng = 0;
		if (update_info.url[0]) {
			leng = http->download(update_info.url,NULL,UPDATE_INI_PATH"update.tar.gz",This->priv->callbackFunc);
			tarUpdateFiles(This->priv->type,UPDATE_INI_PATH);
		}
	}
	return 0;
}

static int uninit(struct _MyUpdate *This)
{
	if (This->priv->type == UPDATE_TYPE_CENTER) {
		if (This->priv->remote)
			This->priv->remote->Destroy(This->priv->remote);
	} else if (This->priv->type == UPDATE_TYPE_SDCARD) {
	} else if (This->priv->type == UPDATE_TYPE_SERVER) {
	}
	return 0;
}

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

static void* threadCheckUpdatInfo(void *arg)
{
	http = myHttpCreate();
	memset(&update_info,0,sizeof(UpdateFileInfo));
	int check_tick = 10; // 启动等待UI10秒后拉取升级数据
	while (1) {
		if (my_update->priv->check_update == 0 && check_tick) {
			check_tick--;
			sleep(1);
			continue;
		}
		int leng = http->download(UPDATE_URL,NULL,UPDATE_INI,NULL);
		if (leng <= 0) {
			sleep(60);
			continue;
		} else {
			char *sec_private[] = {"VersionInfo",NULL};
			my_update->priv->check_update = 0;
			loadIniFile(&update_ini,UPDATE_INI,sec_private);
			if (update_ini) {
				configLoadEtcChar(update_ini,etc_update_char,NELEMENTS(etc_update_char));
				printf("url:%s\n", update_info.url);
				// 判断是否为同一型号产品
				if (strcmp(DEVICE_TYPE,update_info.dev_type))
					break;
				struct VersionType ver_start,ver_end,ver_now;
				getVersionInfo(update_info.start_version,&ver_start.major,&ver_start.minor,&ver_start.release);
				getVersionInfo(update_info.end_version,&ver_end.major,&ver_end.minor,&ver_end.release);
				getVersionInfo(DEVICE_SVERSION,&ver_now.major,&ver_now.minor,&ver_now.release);
				// 判断当前版本是否在要升级的软件版本范围之内
				if (ver_now.major != ver_start.major || ver_now.major != ver_end.major)
					break;
				int start_ver_int = ver_start.minor * 10 + ver_start.release;
				int end_ver_int = ver_end.minor * 10 + ver_end.release;
				int now_ver_int = ver_now.minor * 10 + ver_now.release;
				if (now_ver_int < start_ver_int || now_ver_int > end_ver_int)
					break;
				// 判断是否要强制升级
				if (atoi(update_info.force) == 1) {
					myUpdateStart(UPDATE_TYPE_SERVER,NULL,0,NULL);
				} else
					update_info.need_update = UPDATE_TYPE_SERVER;
			}
			break;
		}
	}
	return NULL;
}

static int needUpdate(struct _MyUpdate *This,char *version,char *content)
{
	This->priv->check_update = 1;	
    if (fileexists(UPDATE_SD)) {
        return UPDATE_TYPE_SDCARD;
    }
	if (update_info.need_update == UPDATE_TYPE_NONE)
		goto need_update_out;

	if (version) {
		strcpy(version,update_info.version);
	}
	if (content) {
		strcpy(content,update_info.readme);
	}
need_update_out:
	return update_info.need_update;	
}

static int isUpdating(struct _MyUpdate *This)
{
	return This->priv->is_updating;	
}

void myUpdateInit(void)
{
	my_update = (MyUpdate *) calloc (1,sizeof(MyUpdate));
	my_update->priv =(struct _MyUpdatePrivate *) calloc (1,sizeof(struct _MyUpdatePrivate)); 
	my_update->interface =(MyUpdateInterface *) calloc (1,sizeof(MyUpdateInterface)); 
	my_update->init = init;
	my_update->download = download;
	my_update->uninit = uninit;
	my_update->needUpdate = needUpdate;
	my_update->isUpdating = isUpdating;
	createThread(threadCheckUpdatInfo,NULL);	
}

static void updateCallback(int result,int reason)
{
	switch(result) 
	{
		case UPDATE_FAIL:
			if (my_update->interface->uiUpdateFail)
				my_update->interface->uiUpdateFail();
			break;
		case UPDATE_SUCCESS:
			if (my_update->interface->uiUpdateDownloadSuccess)
				my_update->interface->uiUpdateDownloadSuccess();
			break;
		case UPDATE_POSITION:
			if (my_update->interface->uiUpdatePos)
				my_update->interface->uiUpdatePos(reason);
			break;
		default:
			break;
	}
}

static void * threadUpdate(void *arg)
{
	my_update->priv->is_updating = 1;
	if (my_update->interface->uiUpdateStart)
		my_update->interface->uiUpdateStart();
	my_update->download(my_update);
	my_update->uninit(my_update);
	if (my_update->interface->uiUpdateStop)
		my_update->interface->uiUpdateStop();
	my_update->priv->is_updating = 0;
	return NULL;
}

void myUpdateStart(int type,char *ip,int port,char *file_path)
{
	if (my_update == NULL)
		return;
	my_update->init(my_update,type,ip,port,file_path,updateCallback);
	createThread(threadUpdate,NULL);	
}
