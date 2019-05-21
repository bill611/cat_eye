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

static size_t downloadCallBackData(void *buffer, size_t size, size_t nmemb, void *user_p)
{
	return	fwrite(buffer, size, nmemb, user_p);
}
static size_t postCallBackData(void *buffer, size_t size, size_t nmemb, void *user_p)
{
	size_t realsize = size * nmemb;
	struct MemoryStruct *mem = (struct MemoryStruct *)user_p;

	mem->memory = realloc(mem->memory, mem->size + realsize + 1);
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

static int download(char *url, char *para, char *file_path)
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
	curl_easy_setopt(easy_handle,CURLOPT_URL,url);
	// curl_easy_setopt(easy_handle, CURLOPT_POST, 1);
	if (para)
		curl_easy_setopt(easy_handle, CURLOPT_POSTFIELDS, para);

	 FILE *outfile;
	 outfile = fopen(file_path, "wb");
	 curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, file_path);
	 curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, &downloadCallBackData);
	 curl_easy_setopt(easy_handle, CURLOPT_SSL_VERIFYHOST, 0L);
	 r = curl_easy_setopt(easy_handle, CURLOPT_SSL_VERIFYPEER, 0L);

	if (r == CURLE_OK) {
		printf("CURLOPT_SSL_VERIFYPEER  sucess!!!\n");
	} else {
		printf("CURLOPT_SSL_VERIFYPEER  failed!!!\n");
		fprintf(stderr, "%s\n", curl_easy_strerror(r));
		goto EXIT;
	}

EXIT:
	curl_easy_cleanup(easy_handle);
	return 0;
}
static int post(char *url, char *para, char *out_data)
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
	curl_easy_setopt(easy_handle,CURLOPT_URL,url);
	// curl_easy_setopt(easy_handle, CURLOPT_POST, 1);
	if (para)
		curl_easy_setopt(easy_handle, CURLOPT_POSTFIELDS, para);
	struct MemoryStruct chunk;
	chunk.memory = malloc(1);  /* will be grown as needed by the realloc above */
	chunk.size = 0;    /* no data at this point */
	curl_easy_setopt(easy_handle, CURLOPT_WRITEFUNCTION, &postCallBackData);
	curl_easy_setopt(easy_handle, CURLOPT_WRITEDATA, (void *)&chunk);

	curl_easy_setopt(easy_handle, CURLOPT_SSL_VERIFYHOST, 0L);
	r = curl_easy_setopt(easy_handle, CURLOPT_SSL_VERIFYPEER, 0L);
	if (r != CURLE_OK) {
		printf("CURLOPT_SSL_VERIFYPEER  failed!!!\n");
		fprintf(stderr, "%s\n", curl_easy_strerror(r));
		goto EXIT;
	}
	r = curl_easy_perform(easy_handle);

	if (r != CURLE_OK) {
		printf("libCurlGetHttp curl_easy_perform failed!!!\n");
		fprintf(stderr, "%s\n", curl_easy_strerror(r));
		goto EXIT;
	}
	// printf("post:%s\n",chunk.memory);
	if(out_data)
       memcpy(out_data, chunk.memory, chunk.size);

EXIT:
	curl_easy_cleanup(easy_handle);
	return 0;
}
MyHttp * myHttpCreate(void)
{
	// 只创建一次接口
	if (my_http)
		return my_http;
	my_http = (MyHttp *) calloc(1,sizeof(MyHttp));
	my_http->post = post;
	my_http->download = download;


	curl_global_init(CURL_GLOBAL_ALL);
	return my_http;
}
