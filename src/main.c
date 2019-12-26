/*
 * =============================================================================
 *
 *       Filename:  main.c
 *
 *    Description:  智能猫眼 
 *
 *        Version:  1.0
 *        Created:  2019-04-19 16:47:01
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
#include <stdint.h>
#include <string.h>
#include <unistd.h>

#include "debug.h"
#include "my_gpio.h"
#include "my_update.h"
#include "protocol.h"
#include "sql_handle.h"
#include "config.h"
#include "my_video.h"
#include "my_mixer.h"
#include "sensor_detector.h"
#include "form_videolayer.h"


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


/* ---------------------------------------------------------------------------*/
/**
 * @brief MiniGUIMain 主函数入口
 *
 * @param argc
 * @param argv[]
 *
 * @returns
 */
/* ---------------------------------------------------------------------------*/
int MiniGUIMain(int argc, const char* argv[])
{
	debugInit();
	configLoad();
	saveLog("stat--->%s,%s\n",DEVICE_SVERSION,g_config.k_version);
	sqlInit();
	gpioInit();
	myMixerInit();
	myVideoInit();
	myUpdateInit();
	sensorDetectorInit();
	protocolInit();
	formVideoLayerCreate();
	return 0;
}
