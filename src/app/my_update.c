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
#include <stdlib.h>
#include <unistd.h>
#include "thread_helper.h"
#include "externfunc.h"
#include "my_update.h"
#include "remotefile.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
struct _MyUpdatePrivate {
	char ip[16];
	char file_path[512];
	int port;
	int type;
	TRemoteFile * remote;
	UpdateFunc callbackFunc;
};

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
MyUpdate *my_update = NULL;

static int init(struct _MyUpdate * This,int type,char *ip,int port,char *file_path,UpdateFunc callbackFunc)
{
	This->priv->type = type;	
	This->priv->port = port;	
	if (ip)
		strcpy(This->priv->ip,ip);
	if (file_path)
		strcpy(This->priv->file_path,file_path);
	This->priv->callbackFunc = callbackFunc;	
	This->priv->remote =  CreateRemoteFile(ip,"Update1.cab");
	return 0;
}
static int download(struct _MyUpdate *This)
{
	if (my_update->priv->type == UPDATE_TYPE_CENTER) {
		This->priv->remote->Download(This->priv->remote,
				0,This->priv->file_path,"/tmp/update.tar.gz",FALSE,This->priv->callbackFunc);
		int ret = system("tar xzf /tmp/update.tar.gz -C /tmp");
		if (ret == 0 && fileexists("/tmp/update/go.sh")) {
			system("/tmp/update/go.sh");
			if (my_update->interface->uiUpdateSuccess)	
				my_update->interface->uiUpdateSuccess();
			sync();
			sleep(1);
			exit(0);
		} else {
			if (my_update->interface->uiUpdateFail)
				my_update->interface->uiUpdateFail();
		}
	} else if (my_update->priv->type == UPDATE_TYPE_SDCARD) {
	} else if (my_update->priv->type == UPDATE_TYPE_SERVER) {
		
	}
}

static int uninit(struct _MyUpdate *This)
{
	This->priv->remote->Destroy(This->priv->remote);
	return 0;
}

void myUpdateInit(void)
{
	my_update = (MyUpdate *) calloc (1,sizeof(MyUpdate));
	my_update->priv =(struct _MyUpdatePrivate *) calloc (1,sizeof(struct _MyUpdatePrivate)); 
	my_update->interface =(MyUpdateInterface *) calloc (1,sizeof(MyUpdateInterface)); 
	my_update->init = init;
	my_update->download = download;
	my_update->uninit = uninit;
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
	if (my_update->interface->uiUpdateStart)
		my_update->interface->uiUpdateStart();
	my_update->download(my_update);
	my_update->uninit(my_update);
	if (my_update->interface->uiUpdateStop)
		my_update->interface->uiUpdateStop();
	return NULL;
}

void myUpdateStart(int type,char *ip,int port,char *file_path)
{
	if (my_update == NULL)
		return;
	my_update->init(my_update,type,ip,port,file_path,updateCallback);
	createThread(threadUpdate,NULL);	
}
