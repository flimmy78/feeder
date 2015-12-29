/*************************************************************************/
/*
 * Copyright (c) 2015, 郑州瑞能电气 Runner All rights reserved.
 *
 * file       : fa.h
 * Design     : zlb
 * Email	  : longbiao831@qq.com
 * Data       : 2015-7-28
 * Version    : V1.0
 * Change     :
 * Description: FA保护结构体类型定义与函数部分声明头文件
 */
/*************************************************************************/
/* xdctools header files */
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Clock.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Event.h>

#include "communicate.h"
#include "collect.h"
#include "sht11.h"
#include "queue.h"
#include "led.h"
#include "fft.h"
#include "sys.h"
#include "log.h"
#include "fa.h"

/****************************** extern define ************************************/


/******************************* private data ***********************************/
UChar ReCounter = 0;				//重合闸次数
UChar old_fastatus = NOPOWER;
/******************************* functions **************************************/
/***************************************************************************/
//函数: UChar Check_PowerON(YCValueStr *faparm, CurrentPaStr *ycvalue)
//说明:	检测上电状态
//输入: 
//输出: 无
//编辑:
//时间:2015.8.17
/***************************************************************************/
static Char OpenDoor(void)
{
	SYSPARAMSTR *sysparm = (SYSPARAMSTR *)ShareRegionAddr.sysconf_addr;
	// 执行动作
	YK_SendOut(0, PIN_LOW);
	// 执行合闸
	YK_SendOut(2, PIN_LOW);
	Task_sleep(sysparm->yc1_out);
//	Task_sleep(200);				
	YK_SendOut(2, PIN_HIGH);
	// 取消预置
	YK_SendOut(0, PIN_HIGH);

	return 0;
}

/***************************************************************************/
//函数: UChar Check_PowerON(YCValueStr *faparm, CurrentPaStr *ycvalue)
//说明:	检测上电状态
//输入: 
//输出: 无
//编辑:
//时间:2015.8.17
/***************************************************************************/
UChar Check_PowerON(YCValueStr *faparm, CurrentPaStr *ycvalue)
{
	UChar status = 0;
	static UChar flag = 0;
	UInt32 channel = 0;
	UInt32 *yxdata = (UInt32 *)ShareRegionAddr.digitIn_addr;
	float lowvol = faparm->faparm->lowvol;
	float lowcur = faparm->faparm->lowcur;
	
	/* 是否超出失限电压 若超出失限电压但是状态不发生切换 */
	if(ycvalue->Ua1.Param > lowvol||ycvalue->Ub1.Param > lowvol
		||ycvalue->Uc1.Param > lowvol)
	{	
		if((flag & 0x01)&&(((yxdata[0] >> (PTPOWER-1)) & 0x01) == 0))
		{
			//send soe PT 有压 //ARM端上传电压值 有压为1  失压为0
			channel = 0x01<<(PTPOWER-1);
			yxdata[0] |= 0x01 << (PTPOWER-1);
			if(((faparm->faparm->softenable >> 17 ) & 0x01)&&((dianbiaodata.yxcos >> (PTPOWER-1)) & 0x01))
			{
				Message_Send(MSG_COS, channel, yxdata[0]);
			}
			if(((faparm->faparm->softenable >> 18 ) & 0x01)&&((dianbiaodata.yxcos >> (PTPOWER-1)) & 0x01))
			{
				Message_Send(MSG_SOE, channel, yxdata[0]);
			}
			//切换状态
			status = 1;
			flag &= 0xfe;
			LOG_INFO("Check_PowerON PT power ua1 = %d, ub1 = %d, uc1 = %d; ",(UInt32)ycvalue->Ua1.Param, (UInt32)ycvalue->Ub1.Param, (UInt32)ycvalue->Uc1.Param);
		}
		else
		{
			/* 超出失限电压时bit0 = 1 */
			flag |= 0x01; 		
		}
	}
	else
	{
		flag &= 0xfe;
		yxdata[0] &= ~(0x01 << (PTPOWER-1));
	}
		
	/* 是否超出失限电流 */
	if(ycvalue->Ia1.Param > lowcur||ycvalue->Ib1.Param > lowcur
		||ycvalue->Ic1.Param > lowcur)
	{
		if(flag & 0x02)
		{
			// 只要电流超过规定值就进入正常状态 进行过流检测
			status = 1;
			//send CT 有流  //ARM端上传电流值  有流为1  失流为0
			channel = 0x01<<(CTPOWER-1);
			yxdata[0] |= 0x01 << (CTPOWER-1);
			if(((faparm->faparm->softenable >> 17 ) & 0x01)&&((dianbiaodata.yxcos >> (CTPOWER-1)) & 0x01))
			{
				Message_Send(MSG_COS, channel, yxdata[0]);
			}
			if(((faparm->faparm->softenable >> 18 ) & 0x01)&&((dianbiaodata.yxcos >> (CTPOWER-1)) & 0x01))
			{
				Message_Send(MSG_SOE, channel, yxdata[0]);
			}
			flag &= 0xfd;
			LOG_INFO("Check_PowerON CT power Ia1 = %d, Ib1 = %d, Ic1 = %d;",(UInt32)ycvalue->Ia1.Param, (UInt32)ycvalue->Ib1.Param, (UInt32)ycvalue->Ic1.Param );
		}
		else
		{
			/* 超出失限电流时bit1 = 1 */
			flag |= 0x02;
			status = 0;
		}
	}
	else
	{
		flag &= 0xfd;
		// 只要CT无流就不会切换状态 清除status标志
		status = 0;
	}
	
	if(flag)
	{
		/* 延时 涌流延时时间 */
		Task_sleep(faparm->faparm->yl_t);
	}

	return status;
}
/***************************************************************************/
//函数: UChar Check_PowerOFF(YCValueStr *faparm, CurrentPaStr *ycvalue)
//说明:	检测失电状态
//输入: 
//输出: 无
//编辑:
//时间:2015.8.17
/***************************************************************************/
UChar Check_PowerOFF(YCValueStr *faparm, CurrentPaStr *ycvalue)
{
	UChar status = 0;
	static UChar flag = 0;
	UInt32 channel = 0;
	static unsigned long times = 0;
	UInt32 *yxdata = (UInt32 *)ShareRegionAddr.digitIn_addr;
	float lowvol = faparm->faparm->lowvol;
	float lowcur = faparm->faparm->lowcur;

	/* 是否低于失限电压 */
	if(ycvalue->Ua1.Param < lowvol && ycvalue->Ub1.Param < lowvol
		&& ycvalue->Uc1.Param < lowvol)
	{	
		if((flag & 0x01)&&((yxdata[0] >> (PTPOWER-1)) & 0x01))
		{
			//send soe PT 有压 //ARM端上传电压值 有压为1  失压为0
			channel = 0x01<<(PTPOWER-1);
			yxdata[0] &= ~(0x01 << (PTPOWER-1));
			if(((faparm->faparm->softenable >> 17 ) & 0x01)&&((dianbiaodata.yxcos >> (PTPOWER-1)) & 0x01))
			{
				Message_Send(MSG_COS, channel, yxdata[0]);
			}
			if(((faparm->faparm->softenable >> 18 ) & 0x01)&&((dianbiaodata.yxcos >> (PTPOWER-1)) & 0x01))
			{
				Message_Send(MSG_SOE, channel, yxdata[0]);
			}	
			status = 1;
			flag &= 0xfe;
			times = 0;
			LOG_INFO("Check_PowerOFF PT power ua1 =  %d, ub1 = %d, uc1 = %d;",(UInt32)ycvalue->Ua1.Param, (UInt32)ycvalue->Ub1.Param, (UInt32)ycvalue->Uc1.Param );
		}
		else
		{
			/* 低于失限电压时bit0 = 1 */
			if((yxdata[0] >> (PTPOWER-1)) & 0x01)
			{
				flag |= 0x01; 	
			}	
		}
	}
	else
	{
		flag &= 0xfe;
	}
	
	/* 是否超出失限电流 */
	if(ycvalue->Ia1.Param < lowcur && ycvalue->Ib1.Param < lowcur
		&&ycvalue->Ic1.Param < lowcur)
	{
		if((flag & 0x02)&&((yxdata[0] >> (CTPOWER-1)) & 0x01))
		{
			status = 1;
			//send CT 有流  //ARM端上传电流值  有流为1  失流为0
			channel = 0x01<<(CTPOWER-1);
			yxdata[0] &= ~(0x01 << (CTPOWER-1));
			if(((faparm->faparm->softenable >> 17 ) & 0x01)&&((dianbiaodata.yxcos >> (CTPOWER-1)) & 0x01))
			{
				Message_Send(MSG_COS, channel, yxdata[0]);
			}
			if(((faparm->faparm->softenable >> 18 ) & 0x01)&&((dianbiaodata.yxcos >> (CTPOWER-1)) & 0x01))
			{
				Message_Send(MSG_SOE, channel, yxdata[0]);
			}
			flag &= 0xfd;
			times = 0;
			LOG_INFO("Check_PowerOFF CT power Ia1 = %d, Ib1 = %d, Ic1 = %d;",(UInt32)ycvalue->Ia1.Param, (UInt32)ycvalue->Ib1.Param, (UInt32)ycvalue->Ic1.Param );
		}
		else
		{
			/* 超出失限电流时bit1 = 1 */
			if((yxdata[0] >> (CTPOWER-1)) & 0x01)
			{
				flag |= 0x02; 	
			}
			status = 0;
		}
	}
	else
	{
		flag &= 0xfd;
		// 只要CT无流就不会切换状态 清除status标志
		status = 0;
	}

	if(flag)
	{
		if(times == 0)
		{
			times = tickets + faparm->faparm->nopower;
			flag = 0;
		}
		else if(times > tickets)
		{
			flag = 0;
		}
	}
	else
	{
		times = 0;
	}

	return status;
}
/***************************************************************************/
//函数:	UChar Check_GL(YCValueStr *faparm, CurrentPaStr *ycvalue, UChar num)
//说明:	过流三段检测  三段num分别为 0 1 2 代表一 二 三段过流
//输入: arg0 arg1
//输出: 无
//编辑:
//时间:2015.8.17
/***************************************************************************/
UChar Check_GL(YCValueStr *faparm, CurrentPaStr *ycvalue, UChar num)
{	
	static UChar GLflag[3] = {0};
	static UChar GLstatus[3] = {0};
	static unsigned long GLtimes[3] = {0};
	static UInt32 GLRECOVERtimes[3] = {0};
	UInt32 channel = 0;
	UInt32 *yxdata = (UInt32 *)ShareRegionAddr.digitIn_addr;
	UChar status = 0;
	
	// A相是否过流
	if((ycvalue->Ia1.Param > faparm->faparm->cursection[num].protectvalue) && ((faparm->faparm->softenable >> 4) & 0x01))
	{
		// A相标记故障
		GLflag[num] |= 0x01 << 0;  			
	}
	else
	{
		GLflag[num] &= ~(0x01 << 0); 
	}
	// B相是否过流
	if((ycvalue->Ib1.Param > faparm->faparm->cursection[num].protectvalue) && ((faparm->faparm->softenable >> 5) & 0x01))
	{
		// B相标记
		GLflag[num] |= 0x01 << 1;
	}
	else
	{
		GLflag[num] &= ~(0x01 << 1); 
	}
	// C相是否过流
	if((ycvalue->Ic1.Param > faparm->faparm->cursection[num].protectvalue) && ((faparm->faparm->softenable >> 6) & 0x01))
	{
		//C相标记
		GLflag[num] |= 0x01 << 2;
	}
	else
	{
		GLflag[num] &= ~(0x01 << 2);
	}

	// 过流故障发生
	if(GLflag[num])
	{	
		if(GLstatus[num] == 0)
		{
			if(GLtimes[num] == 0)
			{
				GLtimes[num] = tickets + faparm->faparm->cursection[num].delaytime;
			}
			else if(GLtimes[num] < tickets)
			{
				//过流标志
				GLstatus[num] = 1;
				GLtimes[num] = 0;
				LEDDATA &= ~(0x01<<LED_PROT);
				LED_SENDOUT(LEDDATA);
				//故障确实发生 产生soe cos
				if(((faparm->faparm->softenable >> 17 ) & 0x01)&&((dianbiaodata.yxcos >> (GLBASE + num -1)) & 0x01))
				{
					channel = 0x01 << (GLBASE + num -1);
					yxdata[0] |= 0x01 << (GLBASE + num -1);
					Message_Send(MSG_COS, channel, yxdata[0]);
				}
				if(((faparm->faparm->softenable >> 18 ) & 0x01)&&((dianbiaodata.yxcos >> (GLBASE + num -1)) & 0x01))
				{
					channel = 0x01 << (GLBASE + num -1);
					yxdata[0] |= 0x01 << (GLBASE + num -1);
					Message_Send(MSG_SOE, channel, yxdata[0]);				
				}
				// 过流跳闸
				if((faparm->faparm->softenable >> 10) & 0x01)
				{
					OpenDoor();	
				}
				status = 1;
				
				LOG_INFO("guoliu num = %d ,GLflag = %d , Ia1 = %d, Ib1 = %d, Ic1 = %d;", 
					num, GLflag[num], (UInt32)ycvalue->Ia1.Param,(UInt32)ycvalue->Ib1.Param , (UInt32)ycvalue->Ic1.Param );
			}
//			LOG_INFO("guoliu num = %d ,GLflag = %d , GLstatus = %d ,Ia1 = %d, Ib1 = %d, Ic1 = %d time = %d;", 
//	num, GLflag[num], GLstatus[num], ycvalue->Ia1.Param,ycvalue->Ic1.Param , ycvalue->Ic1.Param, GLtimes[num] );
		}
		else
		{
			// 重置时间
			GLRECOVERtimes[num] = 0;
			if(old_fastatus == PROTECT)
			{
				// 清除故障标志
				GLstatus[num] = 0;
				
			}
		}
	}
	else
	{
		// 过流状态是否存在
		if(GLstatus[num])
		{
			//重合闸发生时清除故障标志
			if(old_fastatus == PROTECT)
			{
				GLstatus[num] = 0;
			}
			if(GLRECOVERtimes[num] == 0)
			{
				// 故障遥信清除延时
				GLRECOVERtimes[num] = tickets + faparm->faparm->problmeyx * 1000;
			}
			else if(GLRECOVERtimes[num] < tickets || FGflag)  //复归信号发生时可以直接清除故障标志
			{
				//清除过流标志
				GLstatus[num] = 0;
				GLRECOVERtimes[num] = 0;
				//故障失去 产生soe cos
				if(((faparm->faparm->softenable >> 17 ) & 0x01)&&((dianbiaodata.yxcos >> (GLBASE + num -1)) & 0x01))
				{
					channel = 0x01 << (GLBASE + num-1);
					yxdata[0] &= ~(0x01 << (GLBASE + num-1));
					Message_Send(MSG_COS, channel, yxdata[0]);
				}
				if(((faparm->faparm->softenable >> 18 ) & 0x01)&&((dianbiaodata.yxcos >> (GLBASE + num -1)) & 0x01))
				{
					channel = 0x01 << (GLBASE + num-1);
					yxdata[0] &= ~(0x01 << (GLBASE + num-1));
					Message_Send(MSG_SOE, channel, yxdata[0]);				
				}

				if(ReCounter && (old_fastatus == PROTECT))
				{
					// 清除重合闸次数记录
					ReCounter = 0;
				}
				
				LOG_INFO("guoliu num = %d ,GLflag = %d , Ia1 = %d, Ib1 = %d, Ic1 = %d;", 
					num, GLflag[num], (UInt32)ycvalue->Ia1.Param,(UInt32)ycvalue->Ib1.Param , (UInt32)ycvalue->Ic1.Param );	
			}
		}
		else
		{
			// 重置时间
			GLtimes[num] = 0;
		}
	}
	
	return status;
}
/***************************************************************************/
//函数:	UChar Check_LXGL(YCValueStr *faparm, CurrentPaStr *ycvalue, UChar num)
//说明:	过流三段检测  三段num分别为 0 1 2 代表一 二 三段过流
//输入: arg0 arg1
//输出: 无
//编辑:
//时间:2015.8.17
/***************************************************************************/
UChar Check_LXGL(YCValueStr *faparm, CurrentPaStr *ycvalue, UChar num)
{
	static UChar LXGLflag[3] = {0};
	static UChar LXGLstatus[3] = {0};
	static unsigned long LXGLtimes[3] = {0};
	static UInt32 LXGLRECOVERtimes[3] = {0};
	
	UInt32 channel = 0;
	UInt32 *yxdata = ShareRegionAddr.digitIn_addr;
	UChar status = 0;

	if((ycvalue->I01.Param > faparm->faparm->zerosection[num].protectvalue) && ((faparm->faparm->softenable >> 7) & 0x01))
	{
		LXGLflag[num] = 1;
	}
	else 
	{
		LXGLflag[num] = 0;
	}

	if(LXGLflag[num])
	{
		if(LXGLstatus[num] == 0)
		{
			if(LXGLtimes[num] == 0 )
			{
				LXGLtimes[num] = tickets + faparm->faparm->zerosection[num].delaytime;
			}
			else if(LXGLtimes[num] < tickets || FGflag)  //复归信号发生时可以直接清除故障标志
			{
				//过流标志
				LXGLtimes[num] = 0;
				LXGLstatus[num] = 1;
				LEDDATA &= ~(0x01<<LED_PROT);
				LED_SENDOUT(LEDDATA);

				//故障确实发生 产生soe cos
				if(((faparm->faparm->softenable >> 17 ) & 0x01)&&((dianbiaodata.yxcos >> (LXGLBASE + num-1)) & 0x01))
				{
					channel = 0x01 << (LXGLBASE + num-1);
					yxdata[0] |= 0x01 << (LXGLBASE + num-1);
					Message_Send(MSG_COS, channel, yxdata[0]);
				}
				if(((faparm->faparm->softenable >> 18 ) & 0x01)&&((dianbiaodata.yxcos >> (LXGLBASE + num-1)) & 0x01))
				{
					channel = 0x01 << (LXGLBASE + num-1);
					yxdata[0] |= 0x01 << (LXGLBASE + num-1);
					Message_Send(MSG_SOE, channel, yxdata[0]);				
				}
				if((faparm->faparm->softenable >> 10) & 0x01)
				{
					OpenDoor();
				}
				status = 1;

				LOG_INFO("lxguoliu num = %d ,	LXGLflag = %d , I01 = %d", 
					num, LXGLflag[num], (UInt32)ycvalue->I01.Param);
			}	
		}
		else
		{
			LXGLRECOVERtimes[num] = 0;
			if(old_fastatus == PROTECT)
			{
				// 清除故障标志
				LXGLstatus[num] = 0;
			}
		}
	}
	else
	{
		// 过流状态是否存在
		if(LXGLstatus[num])
		{
			//重合闸故障发生时清除故障标志
			if(old_fastatus == PROTECT)
			{
				// 清除故障标志
				LXGLstatus[num] = 0;
			}
			if(LXGLRECOVERtimes[num] == 0)
			{
				// 故障遥信清除延时
				LXGLRECOVERtimes[num] = tickets + faparm->faparm->problmeyx * 1000;
			}
			else if(LXGLRECOVERtimes[num] < tickets)
			{
				//清除过流标志
				LXGLstatus[num] = 0;
				LXGLRECOVERtimes[num] = 0;
				//故障失去 产生soe cos
				if(((faparm->faparm->softenable >> 17 ) & 0x01)&&((dianbiaodata.yxcos >> (LXGLBASE + num-1)) & 0x01))
				{
					channel = 0x01 << (LXGLBASE + num-1);
					yxdata[0] &= ~(0x01 << (LXGLBASE + num-1));
					Message_Send(MSG_COS, channel, yxdata[0]);
				}
				if(((faparm->faparm->softenable >> 18 ) & 0x01)&&((dianbiaodata.yxcos >> (LXGLBASE + num-1)) & 0x01))
				{
					channel = 0x01 << (LXGLBASE + num-1);
					yxdata[0] &= ~(0x01 << (LXGLBASE + num-1));
					Message_Send(MSG_SOE, channel, yxdata[0]);				
				}
				
				if(ReCounter && (old_fastatus == PROTECT))
				{
					// 清除重合闸次数记录
					ReCounter = 0;
				}
				LOG_INFO("guoliu num = %d ,	LXGLflag = %d , I01 = %d", 
					num, LXGLflag[num], (UInt32)ycvalue->I01.Param);
			}
		}
		else
		{
			// 重置时间
			LXGLtimes[num] = 0;
		}
	}

	return status;
}
/***************************************************************************/
//函数: UChar Check_YUEX(YCValueStr *faparm, UChar num)
//说明:	越限保护
//输入: arg0 arg1
//输出: 无
//编辑:
//时间:2015.8.17
/***************************************************************************/
UChar Check_YUEX(YCValueStr *faparm, UChar num)
{
	UChar value = 0;
	UInt32 channel = 0;
	UChar status = 0;
	UInt32 *yxdata = ShareRegionAddr.digitIn_addr;
	float *ycdata = (float *)(ShareRegionAddr.base_addr + faparm->faparm->ycover[num].ycindex *2);
	float yuxvalue = faparm->faparm->ycover[num].value;

	switch(faparm->faparm->ycover[num].flag)
	{	
		case 1:		//上限
			if(ycdata[0] > yuxvalue)
			{
				status = 1;
			}
			else
			{
				status = 0;
			}
			break;
		case 2:		//下限
			if(ycdata[0] < yuxvalue)
			{	
				status = 1;
			}
			else
			{
				status = 0;
			}
			break;
		default:
			break;
	}
	
	if(status && (((yxdata[0] >> (YUEXBASE + num-1)) & 0x01) == 0))
	{
		if(((faparm->faparm->softenable >> 17 ) & 0x01)&&((dianbiaodata.yxcos >> (YUEXBASE + num-1)) & 0x01))
		{
			channel = 0x01 << (YUEXBASE + num-1);
			yxdata[0] |= (0x01 << (YUEXBASE + num-1));
			Message_Send(MSG_COS, channel, yxdata[0]);
		}
		if(((faparm->faparm->softenable >> 18 ) & 0x01)&&((dianbiaodata.yxcos >> (YUEXBASE + num-1)) & 0x01))
		{
			channel = 0x01 << (YUEXBASE + num-1);
			yxdata[0] |= (0x01 << (YUEXBASE + num-1));
			Message_Send(MSG_SOE, channel, yxdata[0]);				
		}
		value = 1;

		LOG_INFO("yuex num = %d ,	ycdata = %d ,yuxvalue = %d ", 
			num,  (UInt32)ycdata[0], yuxvalue);
	}
	else if(status == 0)
	{
		if(yxdata[0] & (0x01 << (YUEXBASE + num-1)))
		{
			channel = 0x01 << (YUEXBASE + num-1);
			yxdata[0] &= ~(0x01 << (YUEXBASE + num-1));
			if(((faparm->faparm->softenable >> 17 ) & 0x01)&&((dianbiaodata.yxcos >> (YUEXBASE + num-1)) & 0x01))
				Message_Send(MSG_COS, channel, yxdata[0]);
			if(((faparm->faparm->softenable >> 18 ) & 0x01)&&((dianbiaodata.yxcos >> (YUEXBASE + num-1)) & 0x01))
				Message_Send(MSG_SOE, channel, yxdata[0]);

			LOG_INFO("yuex num = %d ,	ycdata = %d ,yuxvalue = %d ", 
			num,  (UInt32)ycdata[0], yuxvalue);
		}
	}
	
	return value;
}
/***************************************************************************/
//函数:	Void FA_Task(UArg arg0, UArg arg1) 
//说明:	终端保护任务
//输入: arg0 arg1
//输出: 无
//编辑:
//时间:2015.8.17
/***************************************************************************/
UChar Reclose_State(YCValueStr *faparm)
{
	UChar status = 0;
	UInt32 channel = 0;
	UInt32 *yxstatus = ShareRegionAddr.digitIn_addr;
	SYSPARAMSTR *sysparm = (SYSPARAMSTR *)ShareRegionAddr.sysconf_addr;

	//常闭触点
	if( yxstatus[0] & (0x01 << (FWYX - 1)) )
	{
		// 未储能如果为常开节点 未储能时为低电平//常闭触点
		if((yxstatus[0] >> (WCNYX -1)) & 0x01)
		{
			Task_sleep(faparm->faparm->chargetime);
		}
		// 执行重合闸动作
		if((faparm->faparm->softenable >> 13) & 0x01)
		{
			switch(ReCounter)
			{
				case 0:
					Task_sleep(faparm->faparm->fistreclose_t);
					break;
				case 1:
					Task_sleep(faparm->faparm->secondreclose_t);
					break;
				case 2:
					Task_sleep(faparm->faparm->thirdreclose_t);
					break;
				default:
					Task_sleep(faparm->faparm->thirdreclose_t);
					break;
			}
			// 预置
			YK_SendOut(0, PIN_LOW);
			// 执行合闸
			YK_SendOut(1, PIN_LOW);
			Task_sleep(sysparm->yc1_out);
//			Task_sleep(200);				
			YK_SendOut(1, PIN_HIGH);
			// 取消预置
			YK_SendOut(0, PIN_HIGH);
			// 重合次数自加
			ReCounter++;
			if(((faparm->faparm->softenable >> 17 ) & 0x01)&&((dianbiaodata.yxcos >> (RECLOSE + ReCounter-1)) & 0x01))
			{
				channel = 0x01 << (RECLOSE + ReCounter-1);
				yxstatus[0] |= (0x01 << (RECLOSE + ReCounter-1));
				Message_Send(MSG_COS, channel, yxstatus[0]);
			}
			if(((faparm->faparm->softenable >> 18 ) & 0x01)&&((dianbiaodata.yxcos >> (RECLOSE + ReCounter-1)) & 0x01))
			{
				channel = 0x01 << (RECLOSE + ReCounter-1);
				yxstatus[0] |= (0x01 << (RECLOSE + ReCounter-1));
				Message_Send(MSG_SOE, channel, yxstatus[0]);				
			}
			status = 1;
		}
		else
			status = 1;
	}

	return status;
}
/***************************************************************************/
//函数:	Void FA_Task(UArg arg0, UArg arg1) 
//说明:	终端保护任务
//输入: arg0 arg1
//输出: 无
//编辑:
//时间:2015.8.17
/***************************************************************************/
Void FA_Task(UArg arg0, UArg arg1) 
{
	UChar i;
//	UInt16 count = 0;
	YCValueStr *ycvaluestr;
	UChar fastatus = NOPOWER;
	CurrentPaStr *remotevalue = (CurrentPaStr *)ShareRegionAddr.base_addr;
	UInt32 *yxstatus = (UInt32 *)ShareRegionAddr.digitIn_addr;
	UChar status = 0;

	// 创建fa与fft通信信号量
	fasem = Semaphore_create(0, NULL, NULL); 
	ycvaluestr = &ycvalueprt;
	LOG_INFO("FA_Task creat is ok! ;");

	while(1)
	{
		// 获取FFT信号量
		Semaphore_pend(fasem, BIOS_WAIT_FOREVER);
		switch(fastatus)
		{
			case NOPOWER:
				status = Check_PowerON(ycvaluestr, remotevalue);
				// 切换状态
				if(status)
				{
					old_fastatus = NOPOWER;
					fastatus = 	RUNNING;
					LOG_INFO("Change status to RUNNING! ;");
				}
				break;
			case RUNNING:
				// 分段过流检测
				for(i=0;i<ycvaluestr->faparm->cursection_n;i++)
				{
					status = Check_GL(ycvaluestr, remotevalue, i);
					if(status && ((ycvaluestr->faparm->softenable >> 13) & 0x01))
					{
						old_fastatus = RUNNING;
						fastatus = 	PROTECT;
						break;
					}
//					Task_sleep(10);
				}
				// 零序过流检测
				for(i=0;i<ycvaluestr->faparm->zerosection_n;i++)
				{
					status = Check_LXGL(ycvaluestr, remotevalue, i);
					if(status && ((ycvaluestr->faparm->softenable >> 13) & 0x01))
					{
						old_fastatus = RUNNING;
						fastatus = 	PROTECT;
						break;
					}
				}
				// 越限检测
				for(i=0;i<ycvaluestr->faparm->ycover_n; i++)
				{
					Check_YUEX(ycvaluestr, i);
				}
				// 检测是否失电
				status = Check_PowerOFF(ycvaluestr, remotevalue);
				// 切换状态
				if(status)
				{
					old_fastatus = RUNNING;
					fastatus = 	NOPOWER;
					LOG_INFO("Change status to NOPOWER! ;");
				}
				//故障灯指示
				if(((yxstatus[0] >> (GLBASE - 1))&0x3f) == 0)
				{
					LEDDATA |= 0x01<<LED_PROT;
					LED_SENDOUT(LEDDATA);
				}
				break;
			case PROTECT:
				if(ReCounter < ycvaluestr->faparm->reclose_n)
				{
					status = Reclose_State(ycvaluestr);
					if(status)
					{
						old_fastatus = PROTECT;
						fastatus = RUNNING;
					}		
				}
				else
				{
					// 闭锁
					fastatus = LOCKOUT;
//					LEDDATA |= 0x01<<LED_PROT;
//					LED_SENDOUT(LEDDATA);
				}
				break;
			case LOCKOUT:
				if(FGflag)
				{
					fastatus = RUNNING;
					ReCounter = 0;
				}
				break;
			default:
				break;
		}
	}
		
}
/***************************************************************************/
//函数:	Void Tempture(UArg arg0, UArg arg1)
//说明:	获取温度数据任务
//输入: arg0 arg1
//输出: 无
//编辑:
//时间:2015.8.17
/***************************************************************************/
Void Tempture(UArg arg0, UArg arg1)
{
	CurrentPaStr *remotevalue = (CurrentPaStr *)ShareRegionAddr.base_addr;
	
	while(1)
	{
		// 采集温湿度数据
		GetTempture(remotevalue);
//		LOG_INFO("Tempture is %d ;",remotevalue->DC2.Param);
		Task_sleep(5000);
	}

}









