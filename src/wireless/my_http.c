/*
 * =============================================================================
 *
 *       Filename:  my_http.c
 *
 *    Description:  封装tcp/ip接口
 *
 *        Version:  1.0
 *        Created:  2019-05-21 11:32:29
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
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include "my_http.h"
#include "curl/curl.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
struct MemoryStruct {
	char *memory;
	size_t size;
};

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
MyHttp *my_http = NULL;
static long download_size = 0;  // 当前下载文件大小
static long download_total_size = 0; // 下载文件总大小
static UpdateFunc downloadCallback = NULL; // 下载文件回调函数，用于UI显示

static long getDownloadFileLenth(CURL *handle,const char *url)
{
	double downloadFileLenth = 0;
	curl_easy_setopt(handle, CURLOPT_URL, url);
	curl_easy_setopt(handle, CURLOPT_HEADER, 1);    //只需要header头
	curl_easy_setopt(handle, CURLOPT_NOBODY, 1);    //不需要body
	if (curl_easy_perform(handle) == CURLE_OK) {
		curl_easy_getinfo(handle, CURLINFO_CONTENT_LENGTH_DOWNLOAD, &downloadFileLenth);
	}
	curl_easy_reset(handle);
	return downloadFileLenth;
}

static size_t downloadCallBackData(void *buffer, size_t size, size_t nmemb, void *user_p)
{
	int	pos = 0;
	download_size += nmemb;
	if (download_total_size)
		pos = download_size * 100 / download_total_size;
	if (downloadCallback)
		downloadCallback(UPDATE_POSITION,pos);
	return	fwrite(buffer, size, nmemb, user_p);
}
static size_t postCallBackData(void *buffer, size_t size, size_t nmemb, void *user_p)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)user_p;

	mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) {
		/* out of memory! */
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), buffer, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}
static size_t qiniuPostCallBackData(void *buffer, size_t size, size_t nmemb, void *user_p)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)user_p;

	mem->memory = (char *)realloc(mem->memory, mem->size + realsize + 1);
	if(mem->memory == NULL) {
		/* out of memory! */
		printf("not enough memory (realloc returned NULL)\n");
		return 0;
	}

	memcpy(&(mem->memory[mem->size]), buffer, realsize);
	mem->size += realsize;
	mem->memory[mem->size] = 0;

	return realsize;
}


static int download(char *url, char *para, char *file_path,UpdateFunc callback)
{
	if (!url) {
		printf("NULL url!!\n");
		return -1;
	}
	CURLcode r = CURLE_GOT_NOTHING;
	CURL *easy_handle = curl_easy_init();
	if (!easy_handle) {
		printf("NULL easy_handle!!\n");
		return -1;
	}
	downloadCallback = callback;
	download_total_size = getDownloadFileLenth(easy_handle,url);
	curl_easy_setopt(easy_handle,CURLOPT_URL,url);
	// curl_easy_setopt(easy_handle, CURLOPT_POST, 1);
	if (para)
		curl_easy_setopt(easy_handle, CURLOPT_POSTFIELDS, para);

	 download_size = 0;
	 FILE *fd = NULL;
	 fd = fopen(file_path, "wb");
     if (fd == NULL)
		goto EXIT;
	 curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, fd);
	 curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, &downloadCallBackData);
	 curl_easy_setopt(easy_handle, CURLOPT_SSL_VERIFYHOST, 0L);
	 r = curl_easy_setopt(easy_handle, CURLOPT_SSL_VERIFYPEER, 0L);

	if (r != CURLE_OK) {
		printf("curl download CURLOPT_SSL_VERIFYPEER failed :%s\n",curl_easy_strerror(r));
		goto CLOSE_FD_EXIT;
	}
	r = curl_easy_perform(easy_handle);

	if (r != CURLE_OK) {
		printf("curl download failed :%s\n",curl_easy_strerror(r));
	}

CLOSE_FD_EXIT:
    fflush(fd);
    fclose(fd);
    sync();
EXIT:
	curl_easy_cleanup(easy_handle);
	downloadCallback = NULL;
	return download_size;
}
static int post(char *url, char *para, char **out_data)
{
	if (!url) {
		printf("NULL url!!\n");
		return 0;
	}
	CURLcode r = CURLE_GOT_NOTHING;
	CURL *easy_handle = curl_easy_init();
	if (!easy_handle) {
		printf("NULL easy_handle!!\n");
		return 0;
	}
	curl_easy_setopt(easy_handle,CURLOPT_URL,url);
	// curl_easy_setopt(easy_handle, CURLOPT_POST, 1);
	if (para)
		curl_easy_setopt(easy_handle, CURLOPT_POSTFIELDS, para);
	struct MemoryStruct chunk;
	chunk.memory = (char *)malloc(sizeof(char));  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */
	curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, &postCallBackData);
	curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, (void *)&chunk);

	curl_easy_setopt(easy_handle, CURLOPT_SSL_VERIFYHOST, 0L);
	r = curl_easy_setopt(easy_handle, CURLOPT_SSL_VERIFYPEER, 0L);
	if (r != CURLE_OK) {
		if (chunk.memory)
			free(chunk.memory);
		printf("curl post CURLOPT_SSL_VERIFYPEER failed :%s\n",curl_easy_strerror(r));
		goto EXIT;
	}
	r = curl_easy_perform(easy_handle);

	if (r != CURLE_OK) {
		if (chunk.memory)
			free(chunk.memory);
		printf("curl post failed :%s\n",curl_easy_strerror(r));
		goto EXIT;
	}
	*out_data = chunk.memory;

EXIT:
	curl_easy_cleanup(easy_handle);
	return chunk.size;
}

static int qiniuUpload(char *url,
		char *para,
		char *token,
		char *file_path,
		char *file_name,
	   	char **out_data)
{
	if (!url) {
		printf("NULL url!!\n");
		return 0;
	}
	struct curl_httppost*	formpost	= NULL;
	struct curl_httppost*	lastptr		= NULL;
	CURLcode r = CURLE_GOT_NOTHING;
	CURL *easy_handle = curl_easy_init();
	if (!easy_handle) {
		printf("NULL easy_handle!!\n");
		return 0;
	}
	curl_formadd(&formpost, &lastptr,
			CURLFORM_COPYNAME, "key",
			CURLFORM_COPYCONTENTS, file_name, CURLFORM_END);
	curl_formadd(&formpost, &lastptr,
			CURLFORM_COPYNAME, "token",
			CURLFORM_COPYCONTENTS, token, CURLFORM_END);
	curl_formadd(&formpost, &lastptr,
			CURLFORM_COPYNAME, "file",
			CURLFORM_FILE, file_path, CURLFORM_END);
	// 设置表单参数
	curl_easy_setopt(easy_handle, CURLOPT_HTTPPOST, formpost);

	curl_easy_setopt(easy_handle,CURLOPT_URL,url);
	// curl_easy_setopt(easy_handle, CURLOPT_POST, 1);
	if (para)
		curl_easy_setopt(easy_handle, CURLOPT_POSTFIELDS, para);
	struct MemoryStruct chunk;
	chunk.memory = (char *)malloc(sizeof(char));  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */
	curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, &qiniuPostCallBackData);
	curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, (void *)&chunk);

	curl_easy_setopt(easy_handle, CURLOPT_SSL_VERIFYHOST, 0L);
	r = curl_easy_setopt(easy_handle, CURLOPT_SSL_VERIFYPEER, 0L);
	if (r != CURLE_OK) {
		if (chunk.memory)
			free(chunk.memory);
		printf("curl post CURLOPT_SSL_VERIFYPEER failed :%s\n",curl_easy_strerror(r));
		goto EXIT;
	}
	r = curl_easy_perform(easy_handle);

	if (r != CURLE_OK) {
		if (chunk.memory)
			free(chunk.memory);
		printf("curl post failed :%s\n",curl_easy_strerror(r));
		goto EXIT;
	}
	*out_data = chunk.memory;

EXIT:
	curl_easy_cleanup(easy_handle);
	return chunk.size;
}
MyHttp * myHttpCreate(void)
{
	// 只创建一次接口
	if (my_http)
		return my_http;
	my_http = (MyHttp *) calloc(1,sizeof(MyHttp));
	my_http->post = post;
	my_http->download = download;
	my_http->qiniuUpload = qiniuUpload;


	curl_global_init(CURL_GLOBAL_ALL);
	return my_http;
}
