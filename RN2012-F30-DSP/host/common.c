/*************************************************************************/
// common.c                                        Version 1.00
//
// 本文件是DTU2.0设备的公共资源
// 编写人:R&N                        email:R&N1110@126.com
//
//  日	   期:2015.04.17
//  注	   释:
/*************************************************************************/
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
#include <sys/time.h>
#include <sys/types.h>
#include <sys/shm.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <unistd.h>
#include <linux/rtc.h>

#include <sys/ioctl.h>
#include <net/if.h>
#include <pthread.h>
#include <signal.h>
#include <semaphore.h>

#include "common.h"
#include "iec101.h"
#include "iec104.h"


UInt8 print_buf[60];
/*******************************************************************************************/
//函数说明:设置RTC时间
//输入:rtc_time指针, microseconds		是微秒精度
//输出:0-->成功   1-->RTC文件打开失败2-->读取失败
/*******************************************************************************************/

unsigned char  Set_Rtc_Time( struct rtc_time  * rtctime, long microseconds)
{
    struct tm * stime;
    struct timeval tv;
    time_t timep,timeq;
    int rec;

    time(&timep);
    stime = localtime(&timep);
    stime->tm_sec = rtctime->tm_sec;
    stime->tm_min = rtctime->tm_min;
    stime->tm_hour = rtctime->tm_hour;
    stime->tm_mday = rtctime->tm_mday;
    stime->tm_mon = rtctime->tm_mon - 1; //9-1
    stime->tm_year =rtctime->tm_year +100;//2015-1900=115
    my_debug("Set system time:%d-%02d-%02d %02d:%02d:%02d ",
		stime->tm_year + 1900, stime->tm_mon + 1, stime->tm_mday,
		stime->tm_hour, stime->tm_min, stime->tm_sec);
    timeq = mktime(stime);
    my_debug("now:%ld  new:%ld", timep, timeq);
    tv.tv_sec = timeq;
    //tv.tv_usec = microseconds;
    tv.tv_usec = 0;

    rec = settimeofday(&tv,(struct timezone *)0);
    //rec = gettimeofday(&tv,NULL);
    system("hwclock -w");
        
    if(rec <0 )
    {
        my_debug("settimeofday is fail!\n");
		return -1;
    }
    else
    {
//        my_debug("Set system time:%d-%02d-%02d %02d:%02d:%02d  %04d",
//		stime->tm_year + 1900, stime->tm_mon + 1, stime->tm_mday,
//		stime->tm_hour, stime->tm_min, stime->tm_sec, tv.tv_usec);
		return 0;
    }
	return 0;
}

/*******************************************************************************************/
//函数说明:获取RTC时间
//输入:rtc_time指针
//输出:微秒
/*******************************************************************************************/

unsigned int  Get_Rtc_Time(struct rtc_time  * rtctime)
{
//	time_t t;
	struct tm *timenow1;
	struct timeval tv;

//下面这种方式也可以获取，只不过精度只是s
//	time(&t);
//	timenow1=localtime(&t);
//	printf("get time: %d-%02d-%02d %02d:%02d:%02d \n",timenow1->tm_year + 1900, timenow1->tm_mon + 1,
//		timenow1->tm_mday,timenow1->tm_hour,timenow1->tm_min,timenow1->tm_sec);
//	timenow1->tm_year = 0;
//	timenow1->tm_mon = 0;
//	timenow1->tm_mday = 0;
//这个获取的精度是微妙

	gettimeofday(&tv,NULL);
	timenow1=localtime(&tv.tv_sec);
//	my_debug("get time: %d-%02d-%02d %02d:%02d:%02d  %4d\n",timenow1->tm_year + 1900, timenow1->tm_mon + 1,
//	timenow1->tm_mday,timenow1->tm_hour,timenow1->tm_min,timenow1->tm_sec, tv.tv_usec);
	rtctime->tm_sec = timenow1->tm_sec;
	rtctime->tm_min  = timenow1->tm_min;
	rtctime->tm_hour = timenow1->tm_hour;
	rtctime->tm_mday  = timenow1->tm_mday;
	rtctime->tm_mon  = timenow1->tm_mon; //9-1
	rtctime->tm_year  = timenow1->tm_year;//2014-1900
	//my_debug("tv_sec:%u:%u",tv.tv_sec,tv.tv_usec);
	return tv.tv_usec;
}

/***************************************************************************/
//函数: int  Init_Logfile( char * file )
//说明:用于初始化Log文件
//输入:file  文件路径
//输出: 创建的文件id
//编辑:R&N1110@126.com
//时间:2015.4.17
/***************************************************************************/
int  Init_Logfile( char * file )
{
    int fd = -1;

    fd = open(file,  O_CREAT | O_TRUNC | O_RDWR, 11666);
    if (fd < 0) {
        my_debug("open %s err\n", file);
        return -1;
    }

/*
    if (lseek(fd, 0, SEEK_SET) < 0) {
        my_debug("lseek %s err", file);
        return -1;
    }
    */
    return  fd;
}

/***************************************************************************/
//函数: int  Init_Logfile( char * file )
//说明:用于初始化SOE文件
//输入:file  文件路径
//输出:创建的文件id
//编辑:R&N
//时间:2015.9.26
/***************************************************************************/
int  Init_SOEfile( char * file )
{
     int fd = -1;
     //FILE *fd=NULL;
     /*
	 fd = fopen(file,  "rw");
	 if(fd == NULL){
			my_debug("open %s err\n", file);
            return -1;
		}
     
     //fseek(fd, 0, SEEK_SET);            //重新定义到开始位置
 	*/
    fd = open(file, O_RDWR, 11666);
    if (fd < 0) {
        my_debug("open %s err\n", file);
        return -1;
    }
    return  fd;
}

/***************************************************************************/
//函数:  void Log_Frame(int fd, unsigned char * buf, unsigned short recvcnt, unsigned short len, unsigned char dir )
//说明:用于向log文件中写入数据
//输入:fd--文件id     buf--要写入的内容指针  recvcnt:从buf[recvcnt]开始写入 长度len
//输出:
//编辑:R&N1110@126.com
//时间:2015.4.17
/***************************************************************************/
 void Log_Frame(int fd, unsigned char * buf, unsigned short recvcnt, unsigned short len, unsigned char dir )
{
	static  unsigned short cout_frame=0;
	unsigned short  i = 0, k=0;
	char * tmp_buf = malloc(len*3+21);
	memset(tmp_buf, 0 , len*3+20);
    
	for(i=0; i<len; i++)
	{
	    k = (i+recvcnt)&0x3FF;

	    if(((buf[k]&0xF)>=0)&&((buf[k]&0xF)<=9))
			tmp_buf[(i*3)+5]=(buf[k]&0xF)+'0';
	    if((((buf[k]>>4)&0xF)>=0)&&(((buf[k]>>4)&0xF)<=9))
			tmp_buf[(i*3)+4] =((buf[k]>>4)&0xF)+'0';

    	    if(((buf[k]&0xF)>=10)&&((buf[k]&0xF)<=15))
    	    {
			tmp_buf[(i*3)+5]=(buf[k]&0xF)+'A'-10;
    	    }
    	    if((((buf[k]>>4)&0x0F)>=10)&&(((buf[k]>>4)&0x0F)<=15))
    	    {
    	    		tmp_buf[(i*3)+4] =((buf[k]>>4)&0xF)+'A'-10;
    	    }
	    tmp_buf[(i*3)+6] = ' ';
	}

	if(dir==SEND_FRAME)
		memcpy(tmp_buf,"Tx: ",4);
	else
		memcpy(tmp_buf,"Rx: ",4);
	my_debug("%s", tmp_buf);
	strcat(tmp_buf, "\n");
    IEC101_Senddata2pc(fd, tmp_buf, strlen(tmp_buf));
    IEC104_Senddata2pc(fd, tmp_buf, strlen(tmp_buf));
    
	//write(fd , &tmp_buf[40],  strlen(&tmp_buf[40])); //log
	write(fd , tmp_buf,  strlen(tmp_buf)); //log
	//my_debug("write data to log:%d",cout_frame);
	free(tmp_buf);
    cout_frame++;
	if(cout_frame>= 100)				//每个log里面最多存储多少个记录s
    {  
        cout_frame=0;
		lseek(fd, 0, SEEK_SET);
       }
}




