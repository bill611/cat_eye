
#include <stdio.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <unistd.h>
#include <stdlib.h>
#include "thread_helper.h"
#include "ipc_server.h"
 
typedef struct _IpcServerPriv {
	char ipc_path[64];
	int listen_fd;
	int send_fd;
	IpcCallback callbackFunc;
}IpcServerPriv;

void waitIpcOpen(char *path)
{
	int fd = -1;
	while (1) {
		fd = access(path,0);
		if (fd != 0) {
			// printf("open ipc :%s fail,error:%s\n",path ,strerror(errno));
			sleep(1);
			continue;
		}
		close(fd);
		return;
	}
}
static int sendData(struct _IpcServer *This,char *path,void *data,int size)
{
	static struct sockaddr_un srv_addr;
	This->priv->send_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(This->priv->send_fd < 0) {
		perror("cannot create communication socket");
		return -1;
	}
	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path,(char *)path);
	//connect server
	int ret = connect(This->priv->send_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
	if(ret == -1) {
		// perror("cannot connect to the server");	
		close(This->priv->send_fd);
		return -1;
	}
	write(This->priv->send_fd, data, size);
	close(This->priv->send_fd);
	return 0;
}
static void* threadIpcRecieved(void *arg)
{
	prctl(PR_SET_NAME, __func__, 0, 0, 0);
	struct sockaddr_un clt_addr;
	IpcServer *This = (IpcServer *)arg;
	int len;
	char data_buf[1024];
	while (1) {
		len = sizeof(clt_addr);
		int com_fd = accept(This->priv->listen_fd,(struct sockaddr*)&clt_addr,&len);
		if(com_fd < 0) {
			perror("cannot accept client connect request");
			continue;
		}
		memset(data_buf, 0, 1024);
		int size = read(com_fd, data_buf, sizeof(data_buf));
		if (This->priv->callbackFunc)
			This->priv->callbackFunc(data_buf,size);
		close(com_fd);
	}
	return NULL;
}
static int ipcInit(IpcServer *This)
{
	struct sockaddr_un srv_addr;
	int ret = -1;
	
	This->priv->listen_fd = socket(PF_UNIX, SOCK_STREAM, 0);
	if(This->priv->listen_fd < 0) {
		perror("cannot create communication socket");
		return -1;
	}
	//set server addr_param
	srv_addr.sun_family = AF_UNIX;
	strcpy(srv_addr.sun_path,This->priv->ipc_path);
	unlink(This->priv->ipc_path);
	//bind sockfd & addr
	ret = bind(This->priv->listen_fd, (struct sockaddr*)&srv_addr, sizeof(srv_addr));
	if(ret == -1) {
		perror("cannot bind server socket");
		close(This->priv->listen_fd);
		unlink(This->priv->ipc_path);
		return -1;
	}
	//listen sockfd
	ret = listen(This->priv->listen_fd,1);
	if(ret == -1) {
		perror("cannot listen the client connect request");
		close(This->priv->listen_fd);
		unlink(This->priv->ipc_path);
		return -1;
	}

	createThread(threadIpcRecieved,This);
	return 0;
}

IpcServer* ipcCreate(const char *path,IpcCallback func)
{
	IpcServer* This = (IpcServer*) calloc(1,sizeof(IpcServer));
	if (This == NULL) {
		perror("err to create ipc this\n");
		return NULL;
	}
	This->priv = (IpcServerPriv*) calloc(1,sizeof(IpcServerPriv));
	if (This->priv == NULL) {
		perror("err to create ipc priv\n");
		free(This);
		return NULL;
	}
	strcpy(This->priv->ipc_path,path);
	This->priv->callbackFunc = func;
	This->sendData = sendData;
	int ret = ipcInit(This);
	if (ret == -1) {
		printf("ipc[%s] init fail\n",path);
		free(This->priv);
		free(This);
		return NULL;
	}
	return This;
}
