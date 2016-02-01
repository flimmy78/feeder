/*************************************************************************/
// monitorpc.c                                       Version 1.00
//// 本文件是DTU2.0与上位机通信的代码
// 编写人  :R&N
// email		  :R&N1110@126.com
//  日	   期:2015.05.20
//  注	   释:
/*************************************************************************/
/* cstdlib header files */
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
#include <net/if.h>
#include<pthread.h>
#include <signal.h>
#include <sys/time.h>
#include <time.h>
#include <linux/rtc.h>
#include<semaphore.h>

/* package header files */
#include <ti/syslink/Std.h>     /* must be first */
#include <ti/syslink/IpcHost.h>
#include <ti/syslink/SysLink.h>
#include <ti/ipc/MultiProc.h>
#include <ti/ipc/SharedRegion.h>

/* local header files */
#include "common.h"
#include "monitorpc.h"
#include "sharearea.h"
#include "iec104.h"

/*******************************LOCAL  FUNCTION*****************************/
static void PC_Proce_Message(UInt8 * buf, UInt8 len);



/*******************************LOCAL  VARIOUS*****************************/
LOCAL_Module                * sys_mod_p;


/***************************************************************************/
//函数: static void PC_Proce_Message(UInt8 * buf)
//说明:用于处理接收到的信息和命令
//输入:buf:接收到的数据    len:接收到的长度
//输出:
//编辑:R&N1110@126.com
//时间:2015.5.20
/***************************************************************************/
static void PC_Proce_Message(UInt8 * buf, UInt8 len)
{
    UInt8  funcode, i;
    struct rtc_time  * stime;
    UInt16 usUSec =0;
    UInt16 data0;

    for(i=0; i<len; i++)
        my_debug("len:%d 0x%x",len, buf[i]);

    if((buf[0]==0xeb)&&(buf[1]==0x90))
    {
        if(buf[len-1]==PC_Check_Sum(&buf[2], len-3))//通过校验
         {
            funcode = buf[3];
            switch (funcode)
            {
                my_debug("funcode:0x%d", funcode);
                case MSG_GET_IEC101:
                    sys_mod_p->pc_flag |=(1<<MSG_GET_IEC101);
                    PC_Send_Data(MSG_GET_IEC101, &buf[4], len-5);
                    break;
                case MSG_GET_IEC104:
                    sys_mod_p->pc_flag |=(1<<MSG_GET_IEC104);
                    PC_Send_Data(MSG_GET_IEC104, &buf[4], len-5);
                    break;
                case MSG_LOG:
                    sys_mod_p->pc_flag |=(1<<MSG_LOG);
                    PC_Send_Data(MSG_LOG, &buf[4], len-5);
                    break;
                case MSG_CHECK_TIME:
                    stime = malloc(sizeof(struct rtc_time));
                    stime->tm_mday =buf[6];
                	stime->tm_mon = buf[5];
                	stime->tm_year = buf[4];
                	stime->tm_hour = buf[7];
                	stime->tm_min = buf[8];
                	stime->tm_sec = buf[9];
                    usUSec = *((UInt16 *)(&buf[10]));
                    Set_Rtc_Time(stime,  usUSec*1000);
                    usUSec = Get_Rtc_Time(stime);
                    buf[4] = stime->tm_year;
                    buf[5] = stime->tm_mday;
                	buf[6] = stime->tm_mon;
                	buf[7] = stime->tm_hour;
                	buf[8] = stime->tm_min;
                	buf[9] = stime->tm_sec;
                    *((UInt16 *)(&buf[10]))= usUSec/1000;
                    buf[12]= PC_Check_Sum(&buf[4], 8);
                    PC_Send_Data(MSG_CHECK_TIME, &buf[4], 8);
                    free(stime);
                    break;
                case MSG_CHECK_I://校准电流命令
                case MSG_CHECK_U://校准电压命令
                    if(len == 8)
                    {
                        data0 = *((UInt16 *)&buf[4]);//index
                        Write_To_Sharearea(data0, TYPE_YC_CHECK_VALUE, buf[6]);
                        Send_Event_By_Message(MSG_JIAOZHUN, data0, buf[6]);
                    }
                    break;
                case MSG_EVENT_COS://COS命令
                    sys_mod_p->pc_flag |=(1<<MSG_EVENT_COS);
                    PC_Send_Data(MSG_EVENT_COS, &buf[4], len-5);
                    break;
                case MSG_EVENT_SOE://SOE命令
                    sys_mod_p->pc_flag |=(1<<MSG_EVENT_SOE);
                    my_debug("pc_flag:%d",sys_mod_p->pc_flag);
                    PC_Send_Data(MSG_EVENT_SOE, &buf[4], len-5);
                    break;
                case MSG_DIANBIAO_YC://单次遥测量
                      PC_Send_YC(buf,len);
                    break;
                case MSG_DIANBIAO_YC_DQ:
                    sys_mod_p->pc_flag |=(1<<MSG_DIANBIAO_YC_DQ);
                    PC_Send_Data(MSG_DIANBIAO_YC_DQ, &buf[4], len-5);
                    break;
                case MSG_DIANBIAO_YX://单次遥信量
                    PC_Send_YX(buf,len);
                    break;
                case MSG_DIANBIAO_YX_DQ:
                    sys_mod_p->pc_flag |=(1<<MSG_DIANBIAO_YX_DQ);
                    PC_Send_Data(MSG_DIANBIAO_YX_DQ, &buf[4], len-5);
                    break;
                case MSG_DIANBIAO_YK:
                    for(i=0; i<len-5; )//buf[4]序列号   buf[5]分合闸状态
                    {
                         Send_Event_By_Message(MSG_YK, buf[4+i], buf[5+i]);
                         i = i+2;
                    }
                    break;
                case MSG_DIANBIAO_YC_DQ_CLOSE:
                    sys_mod_p->pc_flag &=~(1<<MSG_DIANBIAO_YC_DQ);
                    PC_Send_Data(MSG_DIANBIAO_YC_DQ_CLOSE, &buf[4], len-5);
                    break;
                case MSG_DIANBIAO_YX_DQ_CLOSE:
                    sys_mod_p->pc_flag &=~(1<<MSG_DIANBIAO_YX_DQ);
                    PC_Send_Data(MSG_DIANBIAO_YX_DQ_CLOSE, &buf[4], len-5);
                    break;

                case MSG_GET_IEC101_CLOSE:
                    sys_mod_p->pc_flag &=~(1<<MSG_GET_IEC101);
                    PC_Send_Data(MSG_GET_IEC101_CLOSE, &buf[4], len-5);
                    break;
                case MSG_GET_IEC104_CLOSE:
                    sys_mod_p->pc_flag &=~(1<<MSG_GET_IEC104);
                    PC_Send_Data(MSG_GET_IEC104_CLOSE, &buf[4], len-5);
                    break;
                case MSG_LOG_CLOSE:
                    sys_mod_p->pc_flag &=~(1<<MSG_LOG);
                    PC_Send_Data(MSG_LOG_CLOSE, &buf[4], len-5);
                    break;
                case MSG_EVENT_SOE_CLOSE:
                    sys_mod_p->pc_flag &=~(1<<MSG_EVENT_SOE);
                     PC_Send_Data(MSG_EVENT_SOE_CLOSE, &buf[4], len-5);
                    break;
                case MSG_EVENT_COS_CLOSE:
                    sys_mod_p->pc_flag &=~(1<<MSG_EVENT_COS);
                    PC_Send_Data(MSG_EVENT_COS_CLOSE, &buf[4], len-5);
                    break;
                case MSG_ALL_CLOSE:
                    sys_mod_p->pc_flag = 0;
                    PC_Send_Data(MSG_ALL_CLOSE, &buf[4], len-5);
                    break;
                case MSG_ALL_SOE:
                    sys_mod_p->pc_flag |=(1<<MSG_ALL_SOE);
                    Send_Soe_Data(pIEC104_Struct->logsoEfd);
                    sys_mod_p->pc_flag &=~(1<<MSG_ALL_SOE);
                    my_debug("pc_flag:%d ,%d",sys_mod_p->pc_flag,1<<MSG_ALL_SOE);
                    //PC_Send_Data(MSG_ALL_SOE, &buf[4], len-5);
                    break;
                case MSG_RBOOT_BOARD:
                    system("reboot");
                    break;
                default:
                    break;
            }
         }
        else
          my_debug("Check_Sum err");
    }
}

/***************************************************************************/
//函数: UInt8 PC_Check_Sum(UInt8 * buf, UInt8 len)
//说明:用于PC机发送数据的函数
//输入:buf:要发送的数据    len:要发送的长度
//输出:UInt8 校验和
//编辑:R&N1110@126.com
//时间:2015.5.20
/***************************************************************************/
UInt8 PC_Check_Sum(UInt8 * buf, UInt8 len)
{
    UInt8 i=0, ret=0;

    if(len == 0)
        return 0;

    for(i=0; i<len; i++)
        ret = ret + buf[i];

    return ret;
}

/***************************************************************************/
//函数: void PC_Send_Data(UInt8 funcode, UInt8 * buf, UInt8 len)
//说明:用于PC机发送数据的函数
//输入:funcode:功能码   buf:要发送的数据    len:要发送的长度
//输出:
//编辑:R&N1110@126.com
//时间:2015.5.20
/***************************************************************************/
void PC_Send_Data(UInt8 funcode, UInt8 * buf, UInt8 len)
{
    char * sendbuf;

    sendbuf = malloc(len+5);
    sendbuf[0] = 0xeb;
    sendbuf[1] = 0x90;
    sendbuf[2] = len;
    sendbuf[3] = funcode;
    memcpy(&sendbuf[4], buf, len);
    sendbuf[len+4] = PC_Check_Sum(buf, len);
    sendbuf[len+4] += (len+funcode);
    if(sys_mod_p->pc_fd > 0)
       send(sys_mod_p->pc_fd, (char*)sendbuf, len+5, 0);
    free(sendbuf);
}

/***************************************************************************/
//函数: void PC_Creat_connection( void * arg )
//说明:用于网络连接，供上位机使用
//输入:
//输出:
//编辑:R&N1110@126.com
//时间:2015.5.20
/***************************************************************************/
void * PC_Creat_connection( void * arg  )
{
	struct ifreq ifr;
	socklen_t len;
	struct sockaddr_in  my_addr,  their_addr;
	unsigned int  lisnum=2;
	fd_set rfds;
	struct timeval tv;
	int retval, maxfd = -1;
	int on = 1;
    UInt8 buf[40];

    sys_mod_p = (LOCAL_Module *)arg;
    int pc_sock_fd = -1;
    
	if ((pc_sock_fd= socket(AF_INET, SOCK_STREAM, 0)) == -1) {
		my_debug("socket");
		exit(1);
	}
	setsockopt( pc_sock_fd, SOL_SOCKET, SO_REUSEADDR, &on, sizeof(on) );//允许地址立即使用

	 strncpy(ifr.ifr_name, "eth0" , IFNAMSIZ);      //这个网口跟IEC104的网口不能冲突
	 if (ioctl(pc_sock_fd, SIOCGIFADDR, &ifr) == 0)    //SIOCGIFADDR 获取ip address
	 {
		bzero(&my_addr,  sizeof(my_addr));
		memcpy(&my_addr, &ifr.ifr_addr, sizeof(my_addr));
		my_debug("Server pc ip:%s port:16800",inet_ntoa(my_addr.sin_addr));
	 }
	my_addr.sin_family = PF_INET;
	my_addr.sin_port = htons(16800);
	if (bind(pc_sock_fd, (struct sockaddr *) &my_addr, sizeof(struct sockaddr))== -1) {
		my_debug("bind");
		exit(1);
	}
	if (listen(pc_sock_fd, lisnum) == -1) {
		my_debug("listen");
		exit(1);
	}  
	while (1)
	{
		len = sizeof(struct sockaddr);
		if ((sys_mod_p->pc_fd=accept(pc_sock_fd,  (struct sockaddr *) &their_addr, &len)) == -1)
		{
			my_debug("accept");
			exit(errno);
		}
		else
			my_debug("server: got connection from %s, port %d, socket %d", inet_ntoa(their_addr.sin_addr), ntohs(their_addr.sin_port), sys_mod_p->pc_fd);

		while (1)
		{
			FD_ZERO(&rfds);		// 把集合清空
			FD_SET(0, &rfds);		// 把标准输入句柄 0 加入到集合中
			FD_SET(sys_mod_p->pc_fd, &rfds);// 把当前连接句柄 new_fd 加入到集合中
			maxfd = 0;
			if (sys_mod_p->pc_fd > maxfd)
				maxfd = sys_mod_p->pc_fd;
			tv.tv_sec = 10;			// 设置最大等待时间
			tv.tv_usec = 0;

			retval = select(maxfd +1, &rfds, NULL, NULL, &tv);// 开始等待

			if (retval == (-1))
			{
				my_debug("select err:%s", strerror(errno));
				break;
			}
			else if (retval == 0)  //超时等待
			{
				continue;
			}
			else 			        //收到数据
			 {
				if (FD_ISSET(0, &rfds)) 					//中断输入数据
				{
				}
				if (FD_ISSET(sys_mod_p->pc_fd, &rfds))
				{
					len = recv(sys_mod_p->pc_fd, buf, 40, 0);// 接收客户端的消息
					if (len > 0)
					{
					    if(len == -1)
                          break;
                        PC_Proce_Message(buf, len);
					}
					else
					{
						if (len < 0)
							my_debug("receive message fail, err code:%d err msg:%s\n",errno, strerror(errno));
						else
							my_debug("The client exit, stop the connection\n");
						break;
					}
				}
			}
		}
		close(sys_mod_p->pc_fd);// 处理每个新连接上的数据收发结束
		sys_mod_p->pc_fd = -1;
        sys_mod_p->pc_flag = 0;
	}
   close(pc_sock_fd);
   pc_sock_fd = -1;
   pthread_exit("Network_Thread finished!\n");
}


