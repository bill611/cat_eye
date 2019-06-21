/*
 * =============================================================================
 *
 *       Filename:  my_video.c
 *
 *    Description:  视频接口
 *
 *        Version:  1.0
 *        Created:  2019-06-19 10:19:50
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
#include "debug.h"
#include "h264_enc_dec/mpi_enc_api.h"
#include "video_server.h"
#include "my_face.h"
#include "my_video.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
MyVideo *my_video;

static void init(void)
{
	myH264EncInit();
	rkVideoInit();
	myFaceInit();
}
static void start(void)
{
	rkVideoStart();
}
static void stop(void)
{
	rkVideoStop();
}
static void capture(int count)
{
	rkVideoStopCapture();
}
static void recordStart(int count)
{
	rkVideoStartRecord();
}
static void recordStop(void)
{
	rkVideoStopRecord();
}

void myVideoInit(void)
{
	my_video = (MyVideo *) calloc(1,sizeof(MyVideo));
	my_video->init = init;
	my_video->start = start;
	my_video->stop = stop;
	my_video->capture = capture;
	my_video->recordStart = recordStart;
	my_video->init();
}
