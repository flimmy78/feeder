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

#include "readconfig.h"

static unsigned char Get_Faconfig_From_Line(char * line, struct _FAPRMETER_ * item );

//FAPARAM *pFAPARAM;
/***************************************************************************/
//函数: static void Read_Faconfig_Conf(char * fd,_FAPARAM_ * fa)
//说明:读FA取点表
//输入:无
//输出:无
//编辑:work
//时间:2015.8.3
/***************************************************************************/
void Read_Faconfig_Conf(char * fd, struct _FAPRMETER_ * fa)
{
 	FILE *fp=NULL;
 	char  line[56];
	fp = fopen(fd,  "rw");
	 if(fp == NULL){
		my_debug("Fail to open\n");
		}
	 fseek(fp, 0, SEEK_SET); 			//重新定义到开始位置
	 while (fgets(line, 56, fp))
 	{
 		Get_Faconfig_From_Line(line,fa);
 	}
	 fclose(fp);
}
static unsigned char Get_Faconfig_From_Line(char * line,struct _FAPRMETER_ * item)
{
    int  i=0,j=0;
	char * tmp_line=NULL;
	char * token = NULL;
    char * token1 = NULL;
    //int uidata[4];
   // int tdata[8];
   // int gdata[7];
    //int cdata[2];
    //int odata[7];
    //int zdata[7];
    //int ydata[10];
	tmp_line=(char * )strstr((char *)line, "loop1=");//find "mode=" first 
	if(tmp_line!=NULL)
		{
			token = strtok( tmp_line, "#");//"#"查找标记 tmp_line:string.标记部分,返回指向标记的指针
			item->loop= atoi( token+strlen("loop1="));//char turn to int type
			//my_debug("loop1:item->loop:%d\n",item->loop);
			return 0;
		}
    
	tmp_line= NULL;
	tmp_line=(char * )strstr((char *)line, "UI=");//UI=
	if(tmp_line!=NULL)
		{
            token1 = strtok(tmp_line,"#");
            token = strtok(token1+3,"/");
            item->lowcur= atoi(token1+strlen("UI="));
            //my_debug("UI:item->lowcur:%d",item->lowcur);
            while(token!=NULL)  
              {  
              	    i++;  
    				token = strtok(NULL ,"/");
    				if(i==1)
    				{
    					item->lowvol = atoi(token);
                        my_debug("item->lowvol:%d",item->lowvol);
    				}
    				else if(i==2)
    				{
    				    item->remotenum = atoi(token);
                        my_debug("item->remotenum:%d",item->remotenum);
    				}
    				else if(i==3)
    				{
              			item->overvol = atoi(token);
                        my_debug("item->overvol:%d\n",item->overvol);
    				}
    				 //my_debug("UI_token:%s\n",token);
              } 
              return 0;
		}
	tmp_line= NULL;
    tmp_line=(char * )strstr((char *)line, "T=");
	if(tmp_line!=NULL)
		{
			token1= strtok(tmp_line, "#");
            token = strtok(token1+2,"/");
			item->problmeyx = atoi(token1+strlen("T="));
    		my_debug("T:item->problmeyx:%d\n",item->problmeyx);
            while(token!=NULL)  
              {  
              	    j++;  
    				token = strtok(NULL ,"/");
                    if(j==1)
                    {
                                item->fastdelay=atoi(token);
                                my_debug("item->fastdelay:%d",item->fastdelay);
                    }
                    else if(j==2)
                    {
                                item->nopower= atoi(token);
                                my_debug("item->nopower:%d",item->nopower);
                    }
                    
                    else if(j==3)
                    {
                      			item->yl_t= atoi(token);
                                my_debug("item->yl_t:%d",item->yl_t);

                    }
                    else if(j==4)
                    {
                                item->recov_t= atoi(token);
                                my_debug("item->recov_t:%d",item->recov_t);
                    }
                    else if(j==5)
                    {
            					item->recloseok_t= atoi(token);
                                my_debug("item->recloseok_t:%d",item->recloseok_t);
                    }
                    else if(j==6)
                    {
                      			item->lockreclose_t= atoi(token);
                                my_debug("item->lockreclose_t:%d",item->lockreclose_t);
                    }
                    else if(j==7)
                    {
                      			item->ykok_t= atoi(token);
                                my_debug("item->ykok_t:%d\n",item->ykok_t);
                    }
                     //my_debug("token:%s\n",token);
              } 
			return 0;
		}
    
    tmp_line=(char * )strstr((char *)line, "G=");
    if(tmp_line!=NULL)
        {
            int k =0;
            token1= strtok( tmp_line, "#");
            token = strtok(token1+2,"/");
            item->gzlbq_t= atoi(token+strlen("G="));
            my_debug("G:item->gzlbq_t:%d",item->gzlbq_t);
            while(token!=NULL)  
              {  
                    k++;  
                    token = strtok(NULL ,"/");
                    switch(k)
                        {
                            case 1:
                                item->gzlbh_t= atoi(token);
                                my_debug("item->gzlbh_t:%d",item->gzlbh_t);
                            break;
                            
                            case 2:
                                item->reclose_n= atoi(token);
                                my_debug("item->reclose_n:%d",item->reclose_n);
                            break;
                            case 3:
                                item->chargetime= atoi(token);
                                my_debug("item->chargetime:%d",item->chargetime);
                            break;
                            case 4:
                                item->fistreclose_t= atoi(token);
                                my_debug("item->fistreclose_t:%d",item->fistreclose_t);
                            break;
                            case 5:
                                item->secondreclose_t= atoi(token);
                                my_debug("item->secondreclose_t:%d",item->secondreclose_t);
                            break;
                            case 6:
                                item->thirdreclose_t= atoi(token);
                                my_debug("item->thirdreclose_t:%d\n",item->thirdreclose_t);
                            break;
                            
                        }
                     //my_debug("token:%s\n",token);
              } 
              return 0;
            }
      
    
    tmp_line=(char * )strstr((char *)line, "O=");
    if(tmp_line!=NULL)
        {
            int m=0;
            token1= strtok( tmp_line, "#");
            token = strtok(token1+2,"/");
            item->cursection_n= atoi(token1+strlen("O="));
            my_debug("O:item->cursection_n:%d",item->cursection_n);
            while(token!=NULL)  
              {  
                    m++;  
                    token = strtok(NULL ,"/");
                    if(m==1)
                    {
                        item->cursection[0].protectvalue= (int)(atof(token) *1000);
                        my_debug("item->cursection[0].protectvalue:%d",item->cursection[0].protectvalue);
                    }
                    
                    else if(m==2)
                    {
                        item->cursection[0].delaytime= atoi(token);
                        my_debug("item->cursection[0].delaytime:%d",item->cursection[0].delaytime);
                    }
                    
                    else if(m==3)
                    {
                        item->cursection[1].protectvalue= (int)(atof(token) *1000);
                        my_debug("item->cursection[1].protectvalue:%d",item->cursection[1].protectvalue);

                    }
                    
                    else if(m==4)
                    {
                        item->cursection[1].delaytime= atoi(token);
                        my_debug("item->cursection[1].delaytime:%d",item->cursection[1].delaytime);
                    }
                    else if(m==5)
                    {
                        item->cursection[2].protectvalue= (int)(atof(token) *1000);
                        my_debug("item->cursection[2].protectvalue:%d",item->cursection[2].protectvalue);
                    }
                    else if(m==6)
                    {
                        item->cursection[2].delaytime= atoi(token);
                        my_debug("item->cursection[2].delaytime:%d\n",item->cursection[2].delaytime);
                    }
                     //my_debug("token:%s\n",token);
              } 
              return 0;
            //return 1;
        }
    
    tmp_line=(char * )strstr((char *)line, "Z=");
    if(tmp_line!=NULL)
        {
            int t=0;
            token1= strtok( tmp_line, "#");
            token = strtok(token1+2,"/");
            item->zerosection_n= atoi(token1+strlen("Z="));
            my_debug("Z:item->zerosection_n:%d",item->zerosection_n);
            while(token!=NULL)  
                {  
                    t++;  
                    token = strtok(NULL ,"/");
                    if(t==1)
                    {
                        item->zerosection[0].protectvalue=(int)(atof(token) *1000);
                        my_debug("item->zerosection[0].protectvalue:%d",item->zerosection[0].protectvalue);
                    }
                    
                    else if(t==2)
                    {
                        item->zerosection[0].delaytime= atoi(token);
                        my_debug("item->zerosection[0].delaytime:%d",item->zerosection[0].delaytime);
                    }
                    
                    else if(t==3)
                    {
                        item->zerosection[1].protectvalue= (int)(atof(token) *1000);
                        my_debug("item->zerosection[1].protectvalue:%d",item->zerosection[1].protectvalue);

                    }
                    
                    else if(t==4)
                    {
                        item->zerosection[1].delaytime= atoi(token);
                        my_debug("item->zerosection[1].delaytime:%d",item->zerosection[1].delaytime);
                    }
                    
                    else if(t==5)
                    {
                        item->zerosection[2].protectvalue= (int)(atof(token) *1000);
                        my_debug("item->zerosection[2].protectvalue:%d",item->zerosection[2].protectvalue);
                    }
                    
                    else if(t==6)
                    {
                        item->zerosection[2].delaytime= atoi(token);
                        my_debug("item->zerosection[2].delaytime:%d\n",item->zerosection[2].delaytime);
                    }
                   
                     //my_debug("token:%s\n",token);
                  } 
                  return 0;
                //return 1;
            }
    
    tmp_line=(char * )strstr((char *)line, "Y=");
    if(tmp_line!=NULL)
        {
            int y=0;
            token1= strtok( tmp_line, "#");
            token = strtok(token1+2,"/");
            item->ycover_n= atoi(token1+strlen("Y="));
            my_debug("Y:item->ycover_n:%d",item->ycover_n);
			while(token!=NULL)  
              {   
                    token = strtok(NULL ,"/");
                    item->ycover[y].ycindex= atoi(token);
				    token = strtok(NULL ,"/");
				    item->ycover[y].flag= atoi(token);
				    token = strtok(NULL ,"/");
				    item->ycover[y].value= (int)(atof(token) *1000);
					my_debug("item->ycover[y].ycindex :%d item->ycover[y].flag :%d item->ycover[y].value :%d",item->ycover[y].ycindex, item->ycover[y].flag, item->ycover[y].value);
				    y++;
				    if(y >= item->ycover_n)
					break;
				}
		if(item->ycover_n != y)
			item->ycover_n = y;
/*		
            while(token!=NULL)  
              {  
                    y++;  
                    token = strtok(NULL ,"/");
                    if(y==1)
                    {
                        item->ycover[0].ycindex= atoi(token);
                        my_debug("item->ycover[0].ycindex:%d",item->ycover[0].ycindex);
                        
                    }
                    
                    else if(y==2)
                    {
                        item->ycover[0].flag= atoi(token);
                        my_debug("item->ycover[0].flag:%d",item->ycover[0].flag);
                    }
                    
                    else if(y==3)
                    {
                        item->ycover[0].value= atoi(token);
                        my_debug("item->ycover[0].value:%d",item->ycover[0].value);

                    }
                    
                    else if(y==4)
                    {
                        item->ycover[1].ycindex= atoi(token);
                        my_debug("item->ycover[1].ycindex:%d",item->ycover[1].ycindex);
                    }
                    
                    else if(y==5)
                    {
                        item->ycover[1].flag= atoi(token);
                        my_debug("item->ycover[1].flag:%d",item->ycover[1].flag);
                    }
                    
                    else if(y==6)
                    {
                        item->ycover[1].value= atoi(token);
                        my_debug("item->ycover[1].value:%d",item->ycover[1].value);
                    }
                    
                    else if(y==7)
                    {
                        item->ycover[2].ycindex= atoi(token);
                        my_debug("item->ycover[2].ycindex:%d",item->ycover[2].ycindex);
                    }
                    
                    else if(y==8)
                    {
                        item->ycover[2].flag= atoi(token);
                        my_debug("item->ycover[2].flag:%d",item->ycover[2].flag);
                    }
                    else if(y==9)
                    {
                        item->ycover[2].value= atoi(token);
                        my_debug("item->ycover[2].value:%d\n",item->ycover[2].value);
                    }
                     //my_debug("token:%s\n",token);
              } */
              return 0;
        }
    
    tmp_line=(char * )strstr((char *)line, "C=");//find "mode=" first 
	if(tmp_line!=NULL)
		{
			token= strtok( tmp_line, "#");//"#"查找标记 tmp_line:string.标记部分,返回指向标记的指针
            //token = strtok(token1+2,"/");
//            item->softenable= atoi(token+strlen("C="));//char turn to int type
			item->softenable = strtol(token+strlen("C="),NULL,16);
			my_debug("softenable:%x\n",item->softenable);
			return 0;
		}
    Write_Fa_Sharearea(item);
	return 0;
}





