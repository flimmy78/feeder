/*************************************************************************/
/*
 * Copyright (c) 2015, ֣�����ܵ��� Runner All rights reserved.
 *
 * file       : fa.h
 * Design     : zlb
 * Email	  : longbiao831@qq.com
 * Data       : 2015-7-28
 * Version    : V1.0
 * Change     :
 * Description: FA�����ṹ�����Ͷ����뺯����������ͷ�ļ�
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
UChar ReCounter = 0;				//�غ�բ����
UChar old_fastatus = NOPOWER;
/******************************* functions **************************************/
/***************************************************************************/
//����: UChar Check_PowerON(YCValueStr *faparm, CurrentPaStr *ycvalue)
//˵��:	����ϵ�״̬
//����: 
//���: ��
//�༭:
//ʱ��:2015.8.17
/***************************************************************************/
static Char OpenDoor(void)
{
	SYSPARAMSTR *sysparm = (SYSPARAMSTR *)ShareRegionAddr.sysconf_addr;
	// ִ�ж���
	YK_SendOut(0, PIN_LOW);
	// ִ�к�բ
	YK_SendOut(2, PIN_LOW);
	Task_sleep(sysparm->yc1_out);
//	Task_sleep(200);				
	YK_SendOut(2, PIN_HIGH);
	// ȡ��Ԥ��
	YK_SendOut(0, PIN_HIGH);

	return 0;
}

/***************************************************************************/
//����: UChar Check_PowerON(YCValueStr *faparm, CurrentPaStr *ycvalue)
//˵��:	����ϵ�״̬
//����: 
//���: ��
//�༭:
//ʱ��:2015.8.17
/***************************************************************************/
UChar Check_PowerON(YCValueStr *faparm, CurrentPaStr *ycvalue)
{
	UChar status = 0;
	static UChar flag = 0;
	UInt32 channel = 0;
	UInt32 *yxdata = (UInt32 *)ShareRegionAddr.digitIn_addr;
	float lowvol = faparm->faparm->lowvol;
	float lowcur = faparm->faparm->lowcur;
	
	/* �Ƿ񳬳�ʧ�޵�ѹ ������ʧ�޵�ѹ����״̬�������л� */
	if(ycvalue->Ua1.Param > lowvol||ycvalue->Ub1.Param > lowvol
		||ycvalue->Uc1.Param > lowvol)
	{	
		if((flag & 0x01)&&(((yxdata[0] >> (PTPOWER-1)) & 0x01) == 0))
		{
			//send soe PT ��ѹ //ARM���ϴ���ѹֵ ��ѹΪ1  ʧѹΪ0
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
			//�л�״̬
			status = 1;
			flag &= 0xfe;
			LOG_INFO("Check_PowerON PT power ua1 = %d, ub1 = %d, uc1 = %d; ",(UInt32)ycvalue->Ua1.Param, (UInt32)ycvalue->Ub1.Param, (UInt32)ycvalue->Uc1.Param);
		}
		else
		{
			/* ����ʧ�޵�ѹʱbit0 = 1 */
			flag |= 0x01; 		
		}
	}
	else
	{
		flag &= 0xfe;
		yxdata[0] &= ~(0x01 << (PTPOWER-1));
	}
		
	/* �Ƿ񳬳�ʧ�޵��� */
	if(ycvalue->Ia1.Param > lowcur||ycvalue->Ib1.Param > lowcur
		||ycvalue->Ic1.Param > lowcur)
	{
		if(flag & 0x02)
		{
			// ֻҪ���������涨ֵ�ͽ�������״̬ ���й������
			status = 1;
			//send CT ����  //ARM���ϴ�����ֵ  ����Ϊ1  ʧ��Ϊ0
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
			/* ����ʧ�޵���ʱbit1 = 1 */
			flag |= 0x02;
			status = 0;
		}
	}
	else
	{
		flag &= 0xfd;
		// ֻҪCT�����Ͳ����л�״̬ ���status��־
		status = 0;
	}
	
	if(flag)
	{
		/* ��ʱ ӿ����ʱʱ�� */
		Task_sleep(faparm->faparm->yl_t);
	}

	return status;
}
/***************************************************************************/
//����: UChar Check_PowerOFF(YCValueStr *faparm, CurrentPaStr *ycvalue)
//˵��:	���ʧ��״̬
//����: 
//���: ��
//�༭:
//ʱ��:2015.8.17
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

	/* �Ƿ����ʧ�޵�ѹ */
	if(ycvalue->Ua1.Param < lowvol && ycvalue->Ub1.Param < lowvol
		&& ycvalue->Uc1.Param < lowvol)
	{	
		if((flag & 0x01)&&((yxdata[0] >> (PTPOWER-1)) & 0x01))
		{
			//send soe PT ��ѹ //ARM���ϴ���ѹֵ ��ѹΪ1  ʧѹΪ0
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
			/* ����ʧ�޵�ѹʱbit0 = 1 */
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
	
	/* �Ƿ񳬳�ʧ�޵��� */
	if(ycvalue->Ia1.Param < lowcur && ycvalue->Ib1.Param < lowcur
		&&ycvalue->Ic1.Param < lowcur)
	{
		if((flag & 0x02)&&((yxdata[0] >> (CTPOWER-1)) & 0x01))
		{
			status = 1;
			//send CT ����  //ARM���ϴ�����ֵ  ����Ϊ1  ʧ��Ϊ0
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
			/* ����ʧ�޵���ʱbit1 = 1 */
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
		// ֻҪCT�����Ͳ����л�״̬ ���status��־
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
//����:	UChar Check_GL(YCValueStr *faparm, CurrentPaStr *ycvalue, UChar num)
//˵��:	�������μ��  ����num�ֱ�Ϊ 0 1 2 ����һ �� ���ι���
//����: arg0 arg1
//���: ��
//�༭:
//ʱ��:2015.8.17
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
	
	// A���Ƿ����
	if((ycvalue->Ia1.Param > faparm->faparm->cursection[num].protectvalue) && ((faparm->faparm->softenable >> 4) & 0x01))
	{
		// A���ǹ���
		GLflag[num] |= 0x01 << 0;  			
	}
	else
	{
		GLflag[num] &= ~(0x01 << 0); 
	}
	// B���Ƿ����
	if((ycvalue->Ib1.Param > faparm->faparm->cursection[num].protectvalue) && ((faparm->faparm->softenable >> 5) & 0x01))
	{
		// B����
		GLflag[num] |= 0x01 << 1;
	}
	else
	{
		GLflag[num] &= ~(0x01 << 1); 
	}
	// C���Ƿ����
	if((ycvalue->Ic1.Param > faparm->faparm->cursection[num].protectvalue) && ((faparm->faparm->softenable >> 6) & 0x01))
	{
		//C����
		GLflag[num] |= 0x01 << 2;
	}
	else
	{
		GLflag[num] &= ~(0x01 << 2);
	}

	// �������Ϸ���
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
				//������־
				GLstatus[num] = 1;
				GLtimes[num] = 0;
				LEDDATA &= ~(0x01<<LED_PROT);
				LED_SENDOUT(LEDDATA);
				//����ȷʵ���� ����soe cos
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
				// ������բ
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
			// ����ʱ��
			GLRECOVERtimes[num] = 0;
			if(old_fastatus == PROTECT)
			{
				// ������ϱ�־
				GLstatus[num] = 0;
				
			}
		}
	}
	else
	{
		// ����״̬�Ƿ����
		if(GLstatus[num])
		{
			//�غ�բ����ʱ������ϱ�־
			if(old_fastatus == PROTECT)
			{
				GLstatus[num] = 0;
			}
			if(GLRECOVERtimes[num] == 0)
			{
				// ����ң�������ʱ
				GLRECOVERtimes[num] = tickets + faparm->faparm->problmeyx * 1000;
			}
			else if(GLRECOVERtimes[num] < tickets || FGflag)  //�����źŷ���ʱ����ֱ��������ϱ�־
			{
				//���������־
				GLstatus[num] = 0;
				GLRECOVERtimes[num] = 0;
				//����ʧȥ ����soe cos
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
					// ����غ�բ������¼
					ReCounter = 0;
				}
				
				LOG_INFO("guoliu num = %d ,GLflag = %d , Ia1 = %d, Ib1 = %d, Ic1 = %d;", 
					num, GLflag[num], (UInt32)ycvalue->Ia1.Param,(UInt32)ycvalue->Ib1.Param , (UInt32)ycvalue->Ic1.Param );	
			}
		}
		else
		{
			// ����ʱ��
			GLtimes[num] = 0;
		}
	}
	
	return status;
}
/***************************************************************************/
//����:	UChar Check_LXGL(YCValueStr *faparm, CurrentPaStr *ycvalue, UChar num)
//˵��:	�������μ��  ����num�ֱ�Ϊ 0 1 2 ����һ �� ���ι���
//����: arg0 arg1
//���: ��
//�༭:
//ʱ��:2015.8.17
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
			else if(LXGLtimes[num] < tickets || FGflag)  //�����źŷ���ʱ����ֱ��������ϱ�־
			{
				//������־
				LXGLtimes[num] = 0;
				LXGLstatus[num] = 1;
				LEDDATA &= ~(0x01<<LED_PROT);
				LED_SENDOUT(LEDDATA);

				//����ȷʵ���� ����soe cos
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
				// ������ϱ�־
				LXGLstatus[num] = 0;
			}
		}
	}
	else
	{
		// ����״̬�Ƿ����
		if(LXGLstatus[num])
		{
			//�غ�բ���Ϸ���ʱ������ϱ�־
			if(old_fastatus == PROTECT)
			{
				// ������ϱ�־
				LXGLstatus[num] = 0;
			}
			if(LXGLRECOVERtimes[num] == 0)
			{
				// ����ң�������ʱ
				LXGLRECOVERtimes[num] = tickets + faparm->faparm->problmeyx * 1000;
			}
			else if(LXGLRECOVERtimes[num] < tickets)
			{
				//���������־
				LXGLstatus[num] = 0;
				LXGLRECOVERtimes[num] = 0;
				//����ʧȥ ����soe cos
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
					// ����غ�բ������¼
					ReCounter = 0;
				}
				LOG_INFO("guoliu num = %d ,	LXGLflag = %d , I01 = %d", 
					num, LXGLflag[num], (UInt32)ycvalue->I01.Param);
			}
		}
		else
		{
			// ����ʱ��
			LXGLtimes[num] = 0;
		}
	}

	return status;
}
/***************************************************************************/
//����: UChar Check_YUEX(YCValueStr *faparm, UChar num)
//˵��:	Խ�ޱ���
//����: arg0 arg1
//���: ��
//�༭:
//ʱ��:2015.8.17
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
		case 1:		//����
			if(ycdata[0] > yuxvalue)
			{
				status = 1;
			}
			else
			{
				status = 0;
			}
			break;
		case 2:		//����
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
//����:	Void FA_Task(UArg arg0, UArg arg1) 
//˵��:	�ն˱�������
//����: arg0 arg1
//���: ��
//�༭:
//ʱ��:2015.8.17
/***************************************************************************/
UChar Reclose_State(YCValueStr *faparm)
{
	UChar status = 0;
	UInt32 channel = 0;
	UInt32 *yxstatus = ShareRegionAddr.digitIn_addr;
	SYSPARAMSTR *sysparm = (SYSPARAMSTR *)ShareRegionAddr.sysconf_addr;

	//���մ���
	if( yxstatus[0] & (0x01 << (FWYX - 1)) )
	{
		// δ�������Ϊ�����ڵ� δ����ʱΪ�͵�ƽ//���մ���
		if((yxstatus[0] >> (WCNYX -1)) & 0x01)
		{
			Task_sleep(faparm->faparm->chargetime);
		}
		// ִ���غ�բ����
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
			// Ԥ��
			YK_SendOut(0, PIN_LOW);
			// ִ�к�բ
			YK_SendOut(1, PIN_LOW);
			Task_sleep(sysparm->yc1_out);
//			Task_sleep(200);				
			YK_SendOut(1, PIN_HIGH);
			// ȡ��Ԥ��
			YK_SendOut(0, PIN_HIGH);
			// �غϴ����Լ�
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
//����:	Void FA_Task(UArg arg0, UArg arg1) 
//˵��:	�ն˱�������
//����: arg0 arg1
//���: ��
//�༭:
//ʱ��:2015.8.17
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

	// ����fa��fftͨ���ź���
	fasem = Semaphore_create(0, NULL, NULL); 
	ycvaluestr = &ycvalueprt;
	LOG_INFO("FA_Task creat is ok! ;");

	while(1)
	{
		// ��ȡFFT�ź���
		Semaphore_pend(fasem, BIOS_WAIT_FOREVER);
		switch(fastatus)
		{
			case NOPOWER:
				status = Check_PowerON(ycvaluestr, remotevalue);
				// �л�״̬
				if(status)
				{
					old_fastatus = NOPOWER;
					fastatus = 	RUNNING;
					LOG_INFO("Change status to RUNNING! ;");
				}
				break;
			case RUNNING:
				// �ֶι������
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
				// ����������
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
				// Խ�޼��
				for(i=0;i<ycvaluestr->faparm->ycover_n; i++)
				{
					Check_YUEX(ycvaluestr, i);
				}
				// ����Ƿ�ʧ��
				status = Check_PowerOFF(ycvaluestr, remotevalue);
				// �л�״̬
				if(status)
				{
					old_fastatus = RUNNING;
					fastatus = 	NOPOWER;
					LOG_INFO("Change status to NOPOWER! ;");
				}
				//���ϵ�ָʾ
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
					// ����
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
//����:	Void Tempture(UArg arg0, UArg arg1)
//˵��:	��ȡ�¶���������
//����: arg0 arg1
//���: ��
//�༭:
//ʱ��:2015.8.17
/***************************************************************************/
Void Tempture(UArg arg0, UArg arg1)
{
	CurrentPaStr *remotevalue = (CurrentPaStr *)ShareRegionAddr.base_addr;
	
	while(1)
	{
		// �ɼ���ʪ������
		GetTempture(remotevalue);
//		LOG_INFO("Tempture is %d ;",remotevalue->DC2.Param);
		Task_sleep(5000);
	}

}









