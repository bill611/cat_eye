/*
 * =============================================================================
 *
 *       Filename:  hal_gpio.c
 *
 *    Description:  硬件层 GPIO控制
 *
 *        Version:  1.0
 *        Created:  2018-12-12 16:26:55
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
#include <fcntl.h>
#include <sys/ioctl.h>
#include <stdlib.h>
#include <unistd.h>
#include <fcntl.h>
#include "gpio-rv1108.h"
#include "hal_gpio.h"

/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
typedef struct {
    char *ioname;
    int iovalue;
} TioLevelCtrl;


/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static int fd;

void halGpioSetMode(int port_id,char* port_name,int dir)
{
	if (fd < 0) {
		return;
	}
#ifdef X86
	return;
#endif
}

void halGpioOut(int port_id,char *port_name,int value)
{
	if (fd < 0) {
		return;
	}
#ifdef X86
    TioLevelCtrl sbuf;
	sbuf.ioname = port_name;
    sbuf.iovalue = value;
	int ret = ioctl(fd, RV1108_IOCGPIO_CTRL, &sbuf);
	if(ret >= 0)
		printf("success\n");
	return;
#endif
}

int halGpioIn(int port_id,char *port_name)
{
#ifdef X86
	return 0;
#endif
}
