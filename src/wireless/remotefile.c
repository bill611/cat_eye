#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/types.h>
#include <stdarg.h>
#include <dirent.h>
#include "tcp_client.h"
#include "remotefile.h"

#define MyPostMessage 

#define tcpclient This->Private->client

typedef struct
{
    int Cmd;            //open , delete
    int Mode;			//1 读 2写 3读写
    int FileNameLen;    // filename size
    char FileName[1];   // filename
}FILE_OPEN_STRUCT;

// operate
enum {FILE_OPEN,FILE_DELETE,FILE_DOWNLOAD};
enum {FILE_READ,FILE_WRITE,FILE_SEEK,FILE_SETEND,FILE_CLOSE};

// file read
typedef struct
{
    int Cmd;            //FILE_READ
    int ReadSize;       //Read Size
}FILE_READ_STRUCT;

// file read return
typedef struct
{
    int Size;
    int Data[1];
}FILE_RET_READ;

// file write head
// return successful is writebyte, or fail is zero.
typedef struct
{
    int Cmd;        //FILE_WRITE
    int WriteSize;
}FILE_WRITE_STRUCT;

// file seek
// return file new postion;
typedef struct
{
    int Cmd;        //FILE_SEEK
    int offset;
    int origin;     // 0 file_begin, 1 file_curren, 2 file_end
}FILE_SEEK_STRUCT;

// set end of file
// return 1 if success, 0 if fail;
typedef struct
{
    int Cmd;        //FILE_SETEND
}FILE_SETEND_STRUCT;

struct RemoteFilePrivate		//私有数据
{
	char TmpFile[64];
	char IP[64];
	BOOL IsOpen;
	int Pos;
	TTCPClient *client;
};

int ExcuteCmd(int bWait,char *Cmd,...)
{
	int i;
	FILE *fp;
	int ret;
	va_list argp;
	char *argv;
	char commond[512] = {0};
	strcat(commond,Cmd);
	va_start( argp, Cmd);
	for(i=1;i<20;i++) {
		argv = va_arg(argp,char *);
		if(argv == NULL)
			break;
		strcat(commond," ");
		strcat(commond,argv);
	}
	va_end(argp);
	printf("cmd :%s\n",commond);
	if ((fp = popen(commond, "r") ) == 0) {
		perror("popen");
		return -1;
	}
	if ( (ret = pclose(fp)) == -1 ) {
		printf("close popen file pointer fp error!\n");
		// printf("popen ret is :%d\n", ret);
	}
	return ret;

	// 用此函数导致产生僵尸进程
	// system(commond);
	// return 0;
}
//---------------------------------------------------------------------------
int ChangeFileName(TRemoteFile * This,char *NewName,const char *FileName)
{
	int k=strlen(FileName)-1;
	if(k<=0)
		return 0;
	while(k>0) {
		if(FileName[k]=='/' || FileName[k]=='\\')
			break;
		k--;
	}
	if(k==0) {
		strcpy(NewName,This->Private->TmpFile);
		return 1;
	}
	strncpy(NewName,FileName,k+1);
	strcpy(&NewName[k+1],This->Private->TmpFile);
	return 1;
}
//---------------------------------------------------------------------------
static int RemoteFile_Download(TRemoteFile * This,unsigned int hWnd,const char *SrcFile,
							   const char *DstFile,int ExecuteMode,UpdateFunc callback)
{
	int FailState;
	int Pos,LastPos=0;
	int filesize,iRet,readpos,readsize;
	FILE *hFile;
	char NewFileName[1024];
	char cBuf[1024];
	FILE_OPEN_STRUCT *pBuf = (FILE_OPEN_STRUCT*)cBuf;

	if(This->Private->IsOpen){
		This->Close(This);
	}
	tcpclient = TTCPClient_Create();
	if(tcpclient==NULL) {
		printf("RemoteFile:TCPClient Create fail!\n");
		if (callback)
			callback(UPDATE_FAIL,UPDATE_FAIL_REASON_CREATE);
		return FALSE;
	}
	if(tcpclient->Connect(tcpclient,This->Private->IP,7893,2000)!=0) {
		//重试
		if(tcpclient->Connect(tcpclient,This->Private->IP,7893,2000)!=0) {
			printf("RemoteFile:connect %s fail!\n",This->Private->IP);
			FailState = UPDATE_FAIL_REASON_CONNECT;
			goto error;
		}
	}

	pBuf->Cmd = FILE_DOWNLOAD;
	pBuf->FileNameLen = strlen(SrcFile)+1;
	if(pBuf->FileNameLen>(512-12)) {
		printf("RemoteFile:filename %s is too length!\n",SrcFile);
		FailState = UPDATE_FAIL_REASON_FILENAME;
		goto error;
	}

	pBuf->Mode = 0;
	strcpy(pBuf->FileName,SrcFile);
	tcpclient->SendBuffer(tcpclient,cBuf,12+pBuf->FileNameLen);
	//printf("Jack : remotefile size = %d\n",filesize);
	int tmp = 0;
	filesize = 0;
	if(tmp = tcpclient->RecvBuffer(tcpclient,&filesize,sizeof(filesize),3000)!=sizeof(filesize)) {
		printf("RemoteFile:Open file Recv Data Error! tmp = %d, size = %d\n",tmp,(filesize));
		FailState = UPDATE_FAIL_REASON_RECV;
		goto error;
	}
	if(filesize<0) {
		printf("RemoteFile:[%s] Return filesize %d is Fail!\n",SrcFile,filesize);
		FailState = UPDATE_FAIL_REASON_RECVSIZE;
		goto error;
	}
	This->Private->Pos = 0;

	//先保存到临时文件
	if(ChangeFileName(This,NewFileName,DstFile)==0) {
		FailState = UPDATE_FAIL_REASON_FILENAME;
		goto error;
	}
	//保存文件
	if((hFile=fopen(NewFileName,"wb"))==NULL) {
		printf("Open %s to write error!\n",NewFileName);
		FailState = UPDATE_FAIL_REASON_OPEN;
		goto error;
	}

	readpos = 0;
	while(readpos<filesize) {
		if(filesize-readpos>1024)
			readsize = 1024;
		else
			readsize = filesize-readpos;
		iRet = tcpclient->RecvBuffer(tcpclient,cBuf,readsize,3000);
		if(readsize!=iRet) {
			printf("Want read size %d, Real read size %d, Abort!\n",readsize,iRet);
			break;
		}
		readpos+=readsize;
		iRet = fwrite(cBuf,1,readsize,hFile);
		if(iRet!=readsize) {
			printf("Want write size %d, Real write size %d, Abort!\n",readsize,iRet);
			break;
		}
		Pos = readpos*100/filesize;
		if(Pos!=LastPos) {
			LastPos = Pos;
			if (callback)
				callback(UPDATE_POSITION,Pos);
		}
	}
	fflush(hFile);
	fclose(hFile);
	tcpclient->Destroy(tcpclient);
	if(readpos==filesize) {
		remove(DstFile);
		if(rename(NewFileName,DstFile)==-1) {
			printf("ChangeFileName %s error\n",DstFile);
			FailState = UPDATE_FAIL_REASON_RENAME;
			goto error;
		}
		if(ExecuteMode) {
			ExcuteCmd(1,"chmod","+x",DstFile,NULL);
		}
		printf("Receive %s success\n",SrcFile);
		if (callback)
			callback(UPDATE_SUCCESS,0);
		return TRUE;
	}
	else {
		printf("Receive %s abort!\n",SrcFile);
		FailState = UPDATE_FAIL_REASON_ABORT;
		goto error;
	}
error:
	tcpclient->Destroy(tcpclient);
	if (callback)
		callback(UPDATE_FAIL,FailState);
	return FALSE;
}
//---------------------------------------------------------------------------
static int RemoteFile_Open(TRemoteFile * This,const char *FileName,int OpenMode)
{
	int iRet;
	char cBuf[256];
	FILE_OPEN_STRUCT *pBuf = (FILE_OPEN_STRUCT*)cBuf;
	if(This->Private->IsOpen){
		This->Close(This);
	}
	tcpclient = TTCPClient_Create();
	if(tcpclient==NULL)
		return -1;
	if(tcpclient->Connect(tcpclient,This->Private->IP,7893,3000)!=0) {
		printf("RemoteFile:connect fail!\n");
		goto error;
	}

	pBuf->Cmd = FILE_OPEN;
	pBuf->FileNameLen = strlen(FileName)+1;
	if(pBuf->FileNameLen>(256-12)) {
		printf("RemoteFile:filename is too length!\n");
		goto error;
	}

	pBuf->Mode = OpenMode;
	strcpy(pBuf->FileName,FileName);
	tcpclient->SendBuffer(tcpclient,cBuf,12+pBuf->FileNameLen);
	if(tcpclient->RecvBuffer(tcpclient,&iRet,sizeof(iRet),3000)!=sizeof(iRet)) {
		printf("RemoteFile:Open file Recv Data Error!\n");
		goto error;
	}
	if(iRet<0) {
		printf("RemoteFile:Open File is Fail!\n");
		goto error;
	}
	This->Private->IsOpen = TRUE;
	This->Private->Pos = 0;
	return iRet;
error:
	tcpclient->Destroy(tcpclient);
	return -1;
}
//---------------------------------------------------------------------------
static int RemoteFile_Read(TRemoteFile * This,void *Buffer,int Size)
{
	int RetSize;
	FILE_READ_STRUCT fileread;
	if(!This->Private->IsOpen)
		return 0;
	if(Size<1 && Size>1024*1024)
		return -1;
	fileread.Cmd = FILE_READ;
	fileread.ReadSize = Size;
	tcpclient->SendBuffer(tcpclient,&fileread,sizeof(FILE_READ_STRUCT));
	if(tcpclient->RecvBuffer(tcpclient,&RetSize,sizeof(int),2000)!=sizeof(int))
		return 0;
	if(tcpclient->RecvBuffer(tcpclient,Buffer,RetSize,2000)!=RetSize)
		return 0;
	This->Private->Pos+=RetSize;
	return RetSize;
}
//---------------------------------------------------------------------------
static BOOL RemoteFile_Write(TRemoteFile * This,void *Buffer,int Size)
{
	BOOL RetVal;
	FILE_WRITE_STRUCT filewrite;
	if(!This->Private->IsOpen)
		return FALSE;
	filewrite.Cmd = FILE_WRITE;
	filewrite.WriteSize = Size;
	tcpclient->SendBuffer(tcpclient,&filewrite,sizeof(FILE_WRITE_STRUCT));
	if(tcpclient->SendBuffer(tcpclient,Buffer,Size)!=Size)
		return FALSE;
	if(tcpclient->RecvBuffer(tcpclient,&RetVal,sizeof(RetVal),1500)!=sizeof(RetVal))
		return FALSE;
	if(RetVal)
		This->Private->Pos+=Size;
	return RetVal;
}
//---------------------------------------------------------------------------
static int RemoteFile_Seek(TRemoteFile * This,int offset,int origin)
{
	int NewPos;
	FILE_SEEK_STRUCT fileseek;
	if(!This->Private->IsOpen)
		return FALSE;
	fileseek.Cmd = FILE_SEEK;
	fileseek.offset = offset;
	fileseek.origin = origin;
	if(tcpclient->SendBuffer(tcpclient,&fileseek,sizeof(FILE_SEEK_STRUCT))!=sizeof(FILE_SEEK_STRUCT))
		return -1;
	if(tcpclient->RecvBuffer(tcpclient,&NewPos,sizeof(NewPos),1000)!=sizeof(NewPos))
		return -1;
	This->Private->Pos = NewPos;
	return NewPos;
}
//---------------------------------------------------------------------------
static BOOL RemoteFile_SetEnd(TRemoteFile * This)
{
	BOOL RetVal;
	FILE_SETEND_STRUCT filesetend;
	if(!This->Private->IsOpen)
		return FALSE;
	filesetend.Cmd = FILE_SETEND;
	if(tcpclient->SendBuffer(tcpclient,&filesetend,sizeof(FILE_SETEND_STRUCT))!=sizeof(FILE_SETEND_STRUCT))
		return FALSE;
	if(tcpclient->RecvBuffer(tcpclient,&RetVal,sizeof(RetVal),1000)!=sizeof(RetVal))
		return FALSE;
	return RetVal;
}
//---------------------------------------------------------------------------
static int RemoteFile_GetPos(TRemoteFile * This)
{
	return This->Private->Pos;
}
//---------------------------------------------------------------------------
static void RemoteFile_Close(TRemoteFile * This)
{
	if(This->Private->IsOpen){
		BOOL Cmd = FILE_CLOSE;
		This->Private->IsOpen = FALSE;
		tcpclient->SendBuffer(tcpclient,&Cmd,sizeof(Cmd));
		tcpclient->Destroy(tcpclient);
	}
}
//---------------------------------------------------------------------------
static void RemoteFile_Destroy(TRemoteFile * This)
{
	This->Close(This);
	free(This->Private);
	free(This);
}
//---------------------------------------------------------------------------
TRemoteFile * CreateRemoteFile(const char *IP,const char *TempFile)
{
	TRemoteFile * This = (TRemoteFile*)malloc(sizeof(TRemoteFile));
	if(This==NULL) {
        printf("alloc TRemoteFile memory failt!\n");
        return NULL;
    }
	This->Private = (struct RemoteFilePrivate*)malloc(sizeof(struct RemoteFilePrivate));
	if(This->Private==NULL) {
		free(This);
        printf("alloc TRemoteFile memory failt!\n");
        return NULL;
	}
	memset(This->Private,0,sizeof(struct RemoteFilePrivate));
	strncpy(This->Private->IP,IP,sizeof(This->Private->IP)-1);
	strncpy(This->Private->TmpFile,TempFile,(sizeof(This->Private->TmpFile))-1);
	This->Open = RemoteFile_Open;
	This->Download = RemoteFile_Download;
	This->Read = RemoteFile_Read;
	This->Write = RemoteFile_Write;
	This->Seek = RemoteFile_Seek;
	This->SetEnd = RemoteFile_SetEnd;
	This->GetPos = RemoteFile_GetPos;
	This->Close = RemoteFile_Close;
	This->Destroy = RemoteFile_Destroy;
	return This;
}
//---------------------------------------------------------------------------
