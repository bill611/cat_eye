/*
 * =============================================================================
 *
 *       Filename:  hal_uart.c
 *
 *    Description:  硬件层 串口驱动
 *
 *        Version:  1.0
 *        Created:  2018-12-13 08:44:29
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
#include <strings.h>
#include <string.h>
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <unistd.h>
#include <termios.h>	//termios.tcgetattr(),tcsetattr
#include "hal_uart.h"
/* ---------------------------------------------------------------------------*
 *                  extern variables declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                  internal functions declare
 *----------------------------------------------------------------------------*/

/* ---------------------------------------------------------------------------*
 *                        macro define
 *----------------------------------------------------------------------------*/
#if (defined ANYKA)
#define TTY_DEV "/dev/ttySAK"
#elif (defined NUVOTON) 
#define TTY_DEV "/dev/ttyS"
#else
#define TTY_DEV
#endif

/* ---------------------------------------------------------------------------*
 *                      variables define
 *----------------------------------------------------------------------------*/
static int uartPortSet(int fd, int baudrate, uint8_t data_bit,
		uint8_t parity,
		uint8_t stop_bit)
{
	struct termios termios_old,termios_new;
	int    tmp;
	bzero(&termios_old,sizeof(termios_old));
	bzero(&termios_new,sizeof(termios_new));
	cfmakeraw(&termios_new);
	tcgetattr(fd,&termios_old);

	switch(baudrate)
	{
		case 2400 :  baudrate=B2400;break;
		case 4800 :  baudrate=B4800;break;
		case 9600 :  baudrate=B9600;break;
		case 19200:  baudrate=B19200;break;
		case 38400:  baudrate=B38400;break;
		case 57600:  baudrate=B57600;break;
		case 115200: baudrate=B115200;break;
		default:     baudrate=B9600;
	}
	cfsetispeed(&termios_new,baudrate);
	cfsetospeed(&termios_new,baudrate);
	termios_new.c_cflag |= CLOCAL;
	termios_new.c_cflag |= CREAD;
	termios_new.c_iflag |= IXON|IXOFF|IXANY;
	termios_new.c_cflag&=~CSIZE;

	switch(data_bit){
		case '5' :
			termios_new.c_cflag |= CS5;
		case '6' :
			termios_new.c_cflag |= CS6;
		case '7' :
			termios_new.c_cflag |= CS7;
		default:
			termios_new.c_cflag |= CS8;

	}
	switch(parity)
	{
		default:
		case '0' :
			termios_new.c_cflag &= ~PARENB;
			break;
		case '1' :
			termios_new.c_cflag |= PARENB;
			termios_new.c_cflag &=~PARODD;
			break;
		case '2' :
			termios_new.c_cflag |= PARENB;
			termios_new.c_cflag |= PARODD;
			break;
	}

	if(stop_bit == '2') 
		termios_new.c_cflag |= CSTOPB;                //2 stop bits
	else
		termios_new.c_cflag &=~CSTOPB;             //1 stop bits
	
	termios_new.c_lflag &=~(ICANON|ECHO|ECHOE|ISIG);
	termios_new.c_oflag &= OPOST;                   //
	termios_new.c_cc[VMIN]  = 1;                    //
	termios_new.c_cc[VTIME] = 0;		      //
	termios_new.c_lflag &= (ICANON|ECHO|ECHOE|ISIG);

	tcflush(fd,TCIFLUSH);                              //
	tmp = tcsetattr(fd,TCSANOW,&termios_new);           //
	tcgetattr(fd,&termios_old);
	return(tmp);
}

int halUartOpen(int com,
		int baudrate,
		uint8_t data_bit,
		uint8_t parity,
		uint8_t stop_bit,
		void *callback_func)
{
	int fd;
	char *ptty;

#if (defined X86)
	return 0;
#else
	switch(com){
		case 0:
			ptty = TTY_DEV"0";
			break;
		case 1:
			ptty = TTY_DEV"1";
			break;
		case 2:
			ptty = TTY_DEV"2";
			break;
		default:
			ptty = TTY_DEV"3";
			break;
	}
	fd = open(ptty,O_RDWR|O_NOCTTY|O_NONBLOCK|O_NDELAY);
	uartPortSet(fd,baudrate,data_bit,parity,stop_bit);
	return fd;
#endif
}

int halUartRead(int fd,void *buf,uint32_t size)
{
#if (defined X86)
	return 0;
#else
	return read(fd,buf,size);
#endif
}

int halUartWrite(int fd,void *buf,uint32_t size)
{
#if (defined X86)
	return 0;
#else
	return write(fd,buf,size);
#endif
}
