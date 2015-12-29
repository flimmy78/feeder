#include <errno.h>
#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <string.h>
#include <sys/types.h>
#include <netinet/in.h>
#include <sys/socket.h>
#include <sys/wait.h>
#include <unistd.h>
#include <arpa/inet.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <net/if.h>
#include<pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <linux/rtc.h>
#include <semaphore.h>
#include <termios.h>


#include <ti/syslink/Std.h>     /* must be first */
#include <ti/ipc/SharedRegion.h>
#include <ti/ipc/MessageQ.h>
#include <ti/syslink/utils/IHeap.h>

#include "common.h"
#include "iec101.h"
#include "monitorpc.h"
#include "sharearea.h"

#include "readsysconfig.h"
SYSPARAM * pSysparam_Struct;

static unsigned char  Get_Sysconfig_From_Line(char * line, struct _SYSPARAME_  * item);

static int set_speed_uart2(int fd,  int speed);
static int set_parity_uart2(int fd, int databits, int stopbits, int parity) ;
static int speed_arr2[] = {B230400, B115200, B57600, B38400, B19200, B9600, B4800, B2400, B1800, B1200, B600, B300};
static int name_arr2[]  = {230400,  115200,  57600,  38400, 19200,  9600,  4800,  2400,  1800,  1200,  600,  300};

//static  int SYS_Open_Console(void);

//SYSPARAM *pSYSPARAM;
//SYS_CONFIG * pSYS_CONFIG;
/*
while(1)
{
    Read_Sysconfig_Conf(MODELSYSFILE,pSYSPARAM);
    set_parity(int fd,int databits,int stopbits,int parity);
    set_speed(int fd,int speed);
    if(IEC101_Open_Console() < 0)
	{
		my_debug("IEC101_Open_Console fail\n");
		return -1;
	}
}*/
/***************************************************************************/
//函数: static void Read_Sysconfig_Conf(char * fd,_SYSPARAM_ * sys)
//说明:读Sys取点表
//输入:无
//输出:无
//编辑:R&N
//时间:2015.8.7
/***************************************************************************/
void Read_Sysconfig_Conf(char * fd,struct _SYSPARAME_ * sys)
{
 	FILE *fp=NULL;
 	char  line[31];
	fp = fopen(fd,  "rw");
	 if(fp == NULL){
		my_debug("Fail to open\n");
		}
	 fseek(fp, 0, SEEK_SET); 			//重新定义到开始位置
	 while (fgets(line, 31, fp))
 	{
 		Get_Sysconfig_From_Line(line,sys);
        //my_debug("sysconfig");
 	}
	 fclose(fp);
}

static unsigned char Get_Sysconfig_From_Line(char * line,struct _SYSPARAME_ * item)
{
    //int  i=0;
	char * tmp_line=NULL;
	char * token = NULL;
    //char * token1 = NULL;
	tmp_line=(char * )strstr((char *)line, "yc1_out=");
	if(tmp_line!=NULL)
		{
			token= strtok( tmp_line, "#");
            item->DspReadData.yc1_out= atoi( token+strlen("yc1_out="));
            
			my_debug("item->yc1_out:%d",item->DspReadData.yc1_out);
			return 0;
		}
    
	tmp_line=(char * )strstr((char *)line, "yc2_out=");
	if(tmp_line!=NULL)
		{
			token= strtok( tmp_line, "#");
            item->DspReadData.yc2_out= atoi( token+strlen("yc2_out="));
			my_debug("item->yc2_out:%d",item->DspReadData.yc2_out);
			return 0;
		}
    
	tmp_line=(char * )strstr((char *)line, "yxtime="); 
	if(tmp_line!=NULL)
		{
			token= strtok( tmp_line, "#");
            item->DspReadData.yxtime= atoi( token+strlen("yxtime="));
			my_debug("item->yxtime:%d",item->DspReadData.yxtime);
			return 0;
		}
    
	tmp_line=(char * )strstr((char *)line, "ptrate="); 
	if(tmp_line!=NULL)
		{
			token= strtok( tmp_line, "#");
            item->DspReadData.ptrate= atoi( token+strlen("ptrate="));
			my_debug("item->ptrate:%d",item->DspReadData.ptrate);
			return 0;
		}
	tmp_line=(char * )strstr((char *)line, "pctrate="); 
	if(tmp_line!=NULL)
		{
			token= strtok( tmp_line, "#");
            item->DspReadData.pctrate= atoi( token+strlen("pctrate="));
			my_debug("item->pctrate:%d",item->DspReadData.pctrate);
			return 0;
		}
    
	tmp_line=(char * )strstr((char *)line, "battlecycle="); 
	if(tmp_line!=NULL)
		{
			token= strtok( tmp_line, "#");
            item->DspReadData.battlecycle= atoi( token+strlen("battlecycle="));
			my_debug("item->battlecycle:%d",item->DspReadData.battlecycle);
			return 0;
		}
    
	tmp_line=(char * )strstr((char *)line, "battletime="); 
	if(tmp_line!=NULL)
		{
			token= strtok( tmp_line, "#");
            item->DspReadData.battletime= atoi( token+strlen("battletime="));
			my_debug("item->battletime:%d",item->DspReadData.battletime);
			return 0;
		}
    
    
	tmp_line=(char * )strstr((char *)line, "ctrate="); 
	if(tmp_line!=NULL)
		{
			token= strtok( tmp_line, "#");
            item->DspReadData.ctrate= atoi( token+strlen("ctrate="));
			my_debug("item->ctrate:%d",item->DspReadData.ctrate);
			return 0;
		}
    Write_Sys_Sharearea(item);
		
    /*串口*/
    
	tmp_line=(char * )strstr((char *)line, "uart2="); 
	if(tmp_line!=NULL)
		{
			token= strtok( tmp_line, "#");
            item->uart2= atoi( token+strlen("uart2="));
            my_debug("uart2=%d",item->uart2);
			return 1;
		}

	tmp_line=(char * )strstr((char *)line, "bitrate="); 
	if(tmp_line!=NULL)
		{
			token= strtok( tmp_line, "#");
            item->bitrate= atoi( token+strlen("bitrate="));
            my_debug("bitrate=%d",item->bitrate);
            
			return 1;
		}
    
    
    return 0;
            
}
/*****************************************************************/
//Name:       open_dev
//Function:   打开串口设备
//Input:      无Return:     FALSE TRUE
//Author:shaoyi1110		qq:402097953
//Time:2014.10.20
/******************************************************************/
/*
static int open_uart2(char *dev)
{
	int fd= open(dev, O_RDWR );
    my_debug("open_uart2");
	if (-1 == fd)
	{
		my_debug("Can't Open Serial Port:%s\n",dev);
		return -1;
	}
	else
		return fd;
}
int Uart2_Open_Console(void)
{
	char  file_name[10];
    my_debug("Uart2_Open_Console");
	 memset(file_name, 0 , 10);
	 sprintf(file_name, "/dev/ttyS%d",  pSysparam_Struct->uart2);
	 pSysparam_Struct->fd_comtype= open_uart2( file_name );
	 if(pSysparam_Struct->fd_comtype < 0)
	 	return -1;
 	set_speed_uart2(pSysparam_Struct->fd_comtype, pSysparam_Struct->bitrate);
	set_parity_uart2(pSysparam_Struct->fd_comtype, 8, 1, 'N');
		return 0;
}
*/
/*****************************************************************/
//Name:       set_speed
//Function:   设置波特率
//Input:      无Return:     FALSE TRUE
/******************************************************************/
/*
static int set_speed_uart2(int fd,  int speed)
{
	int i;
	int status;
	struct termios Opt;

	tcgetattr(fd, &Opt);

	for ( i= 0; i < sizeof(speed_arr2) / sizeof(int); i++)
	{
		if (speed == name_arr2[i])
		{
			tcflush(fd, TCIOFLUSH);
			cfsetispeed(&Opt, speed_arr2[i]);
			cfsetospeed(&Opt, speed_arr2[i]);

			status = tcsetattr(fd, TCSANOW, &Opt);
			if (status != 0)
			{
				perror("tcsetattr fd1");
				return -1;
			}

			tcflush(fd,TCIOFLUSH);
			return 0;
		}
    }
    my_debug("set_speed_uart2");
	return -1;
}
*/
/***************************************************
//Name:       set_parityFunction:   设置串口数据位，停止位和效验位
//Input:      1.fd          打开的串口文件句柄
//		 2.databits    数据位  取值  为 7  或者 8
//		 3.stopbits    停止位  取值为 1 或者2
//		 4.parity      效验类型  取值为 N,E,O,S
//Return:
****************************************************/
/*
static int set_parity_uart2(int fd, int databits, int stopbits, int parity)
{
	struct termios options;

	if ( tcgetattr( fd,&options) != 0)
	{
		my_debug("SetupSerial 1");
		return(-1);
    }
	options.c_cflag &= ~CSIZE;

	// 设置数据位数
	switch (databits)
	{
		case 7:
			options.c_cflag |= CS7;
			break;

		case 8:
			options.c_cflag |= CS8;
			break;

		default:
			fprintf(stderr,"Unsupported data sizen");
			return (-1);
	}

	switch (parity)
	{
		case 'n':
		case 'N':
			options.c_cflag &= ~PARENB; 				//Clear parity enable 
			options.c_iflag &= ~INPCK;					// Enable parity checking 
			break;

		case 'o':
		case 'O':
			options.c_cflag |= (PARODD | PARENB);
			options.c_iflag |= INPCK;					// Disnable parity checking 
			break;

		case 'e':
		case 'E':
			options.c_cflag |= PARENB;					// Enable parity 
			options.c_cflag &= ~PARODD;
			options.c_iflag |= INPCK; 					// Disnable parity checking 
			break;

		case 'S':
		case 's': 										//as no parity
			options.c_cflag &= ~PARENB;
			options.c_cflag &= ~CSTOPB;
			break;

		default:
			fprintf(stderr,"Unsupported parityn");
			return (-1);
	}

    //  设置停止位
	switch (stopbits)
	{
		case 1:
			options.c_cflag &= ~CSTOPB;
			break;

		case 2:
			options.c_cflag |= CSTOPB;
			break;

		default:
			my_debug("Unsupported stop bitsn");
			return (-1);
	}

	// Set input parity option 
	if (parity != 'n')
		options.c_iflag |= INPCK;

	tcflush(fd,TCIFLUSH);
	options.c_iflag = 0;
    	options.c_oflag = 0;
    	options.c_lflag = 0;
	options.c_cc[VTIME] = 150;							// 设置超时 15 seconds
	options.c_cc[VMIN] = 0;								// Update the options and do it NOW
	if (tcsetattr(fd,TCSANOW,&options) != 0)
	{
		perror("SetupSerial 3");
		return (-1);
	}
    my_debug("set_parity_uart2");
	return (0);
}*/


