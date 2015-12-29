#include <stdio.h>
#include <stdlib.h>
#include "nand_rw.h"
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
#include "iec104.h"
#include "common.h"
#include "iec101.h"
#include "monitorpc.h"
#include "sharearea.h"
#include "nand_rw.h"
sem_t              			bin_sem;
/***************************************************************************/
//函数: void Nand_Write_Config(char * file)
//说明:write data to nandfile.txt
//输入:
//输出:
//编辑:R&N
//时间:2015.9.6
/***************************************************************************/
void Nand_Write_Config(char * file)
{
    int res;
	res=sem_init(&bin_sem, 0, 0);//创建一个未命名的信号量
	if(res!=0)
    {
        perror("semaphore initialization failed!");
    }   
    my_debug("sem_init\n");
    int fp = -1;
    unsigned char buff[6] = {0};
    buff[0]='r';
    buff[1]=' ';
    buff[2]='w';
    buff[3]=' ';
    buff[4]='s';
    buff[5]='\n';
    fp= open(file,  O_CREAT | O_TRUNC | O_RDWR, 11666);
    if (fp < 0) 
    {
        my_debug("open %s err\n", file);
        return -1;
    }
	if(fp>0)
    {   
        //sem_post(&bin_sem);
        write(fp, buff,6);
    }
     //lseek(fp,0,SEEK_SET);
     my_debug("Write data: buff0=%c buff2=%c buff4=%c\n",buff[0],buff[2],buff[4]);
     //sleep(1);
     close(fp);
     Nand_Read_Config(file);
}
/***************************************************************************/
//函数: void Nand_Read_Config(char * file)
//说明:read data from nandfile.txt
//输入:
//输出:
//编辑:R&N
//时间:2015.9.6
/***************************************************************************/
void Nand_Read_Config(char * file)
{
    int fp = -1;
    unsigned char buffc[6] = {0};
    fp= open(file, O_RDONLY, 11666);
    if (fp < 0) {
        my_debug("open %s err\n", file);
        return -1;
    }
    lseek(fp,0,SEEK_SET);
    if(fp>0)
    {
        //sem_wait(&bin_sem);
        read(fp, buffc,6); //
        my_debug("Read data:buffc0:%c buffc2:%c buffc4:%c\n",buffc[0],buffc[2],buffc[4]);
    }
    close(fp);
}
