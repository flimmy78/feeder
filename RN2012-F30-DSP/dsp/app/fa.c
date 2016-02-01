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
FAPrivateStr faprivatedata;		//fa�ļ�ȫ�ֱ�������
/******************************* functions **************************************/
/***************************************************************************/
//����: static Char OpenDoor(void)
//˵��:	��·���
//����: ��
//���: ��·�Ƿ�ɹ�
//�༭:
//ʱ��:2015.8.17
/***************************************************************************/
static Char OpenDoor(void)
{
	Char err = 0;
	SYSPARAMSTR *sysparm = (SYSPARAMSTR *)ShareRegionAddr.sysconf_addr;

	// ִ��Ԥ�ö���
	YK_SendOut(0, PIN_LOW);
	// ��ʱ�ȴ�Ԥ�ö���ִ�����
	Task_sleep(20);
	if(GPIOPinRead(SOC_GPIO_0_REGS, YK_YZ))	
		return -1;
	// ִ�з�բ����
	YK_SendOut(2, PIN_LOW);
	// ��ʱִ��ʱ��
	Task_sleep(sysparm->yc1_out);
	// ��բ״̬Ϊ����,YX_1Ϊ��բ״̬
	if(GPIOPinRead(SOC_GPIO_0_REGS, YX_1))
	{
		//��բʧ��
		err = 1;
		LOG_INFO("open is err by yx. ");
	}
	else
	{
		//��բ�ɹ�
		err = 0;		
	}			
	YK_SendOut(2, PIN_HIGH);
	// ȡ��Ԥ��
	YK_SendOut(0, PIN_HIGH);

	LOG_INFO("open is right. ");
	return err;
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
	UInt32 problem = 0;
	float lowvol = faparm->faparm->lowvol*1000;
	float lowcur = faparm->faparm->lowcur*1000;

	if(ycvalue->Ua1.Param > lowvol||ycvalue->Ub1.Param > lowvol
		||ycvalue->Uc1.Param > lowvol)
	{
		//�ж��Ƿ���λ���
		if(flag & 0x01)
		{
			//Ĭ��1Ϊ����״̬
			problem = (yxstatus >> PTNOPOWER) & 0x01;
			//�ж��Ƿ�Ϊ����״̬,ͬΪ����״̬���ٽ����¼��ϱ�
			if(problem == 0)
			{
				yxstatus |= 0x01 << PTNOPOWER; 
				//send soe PT ��ѹ,����ʹ���ź�
				Send_CS(PTNOPOWER);
				//�л�״̬ʹ��
				status = 1;
				//�����־λ
				flag &= 0xfe;
				LOG_INFO("Check_PowerON PT power ua1 = %d, ub1 = %d, uc1 = %d; ",(UInt32)ycvalue->Ua1.Param, (UInt32)ycvalue->Ub1.Param, (UInt32)ycvalue->Uc1.Param);
			}
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
		//��ѹʱ�ָ�״̬
		yxstatus &= ~(0x01 << PTNOPOWER);
	}
		
	/* �Ƿ񳬳�ʧ�޵��� */
	if(ycvalue->Ia1.Param > lowcur||ycvalue->Ib1.Param > lowcur
		||ycvalue->Ic1.Param > lowcur)
	{
		if(flag & 0x02)
		{
			// ֻҪ���������涨ֵ�ͽ�������״̬ ���й������
			status = 1;
			yxstatus |= 0x01 << CTNOPOWER; 
			//send CT ����  //ARM���ϴ�����ֵ  ����Ϊ1  ʧ��Ϊ0
			Send_CS(CTNOPOWER);
			flag &= 0xfd;
			LOG_INFO("Check_PowerON CT power Ia1 = %d, Ib1 = %d, Ic1 = %d;",(UInt32)ycvalue->Ia1.Param, (UInt32)ycvalue->Ib1.Param, (UInt32)ycvalue->Ic1.Param );
		}
		else
		{
			/* ����ʧ�޵���ʱbit1 = 1 */
			flag |= 0x02;
			//������ѹ�������õı�־λ
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
	UInt32 problem = 0;
	static unsigned long times = 0;
	UInt32 lowvol = faparm->faparm->lowvol*1000;
	UInt32 lowcur = faparm->faparm->lowcur*1000;

	/* �Ƿ����ʧ�޵�ѹ */
	if(ycvalue->Ua1.Param < lowvol && ycvalue->Ub1.Param < lowvol
		&& ycvalue->Uc1.Param < lowvol)
	{	
		//Ĭ��1Ϊ����״̬
		problem = (yxstatus >> PTNOPOWER) & 0x01;
		//�ж��Ƿ�Ϊ����״̬,ͬΪ����״̬���ٽ����¼��ϱ�	
		if(problem)
		{
			if(flag & 0x01)
			{	
				yxstatus &= ~(0x01 << PTNOPOWER);
				//send soe PT ��ѹ //ARM���ϴ���ѹֵ ��ѹΪ1  ʧѹΪ0
				Send_CS(PTNOPOWER);
				status = 1;
				flag &= 0xfe;
				times = 0;
				LOG_INFO("Check_PowerOFF PT power ua1 =  %d, ub1 = %d, uc1 = %d;",(UInt32)ycvalue->Ua1.Param, (UInt32)ycvalue->Ub1.Param, (UInt32)ycvalue->Uc1.Param );
			}
			else
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
		//Ĭ��1Ϊ����״̬
		problem = (yxstatus >> CTNOPOWER) & 0x01;
		//�ж��Ƿ�Ϊ����״̬,ͬΪ����״̬���ٽ����¼��ϱ�	
		if(problem)
		{
			if(flag & 0x02)
			{	
				yxstatus &= ~(0x01 << CTNOPOWER);
				//send soe PT ��ѹ //ARM���ϴ���ѹֵ ��ѹΪ1  ʧѹΪ0
				Send_CS(CTNOPOWER);
				// ���ϱ���������,ֻ�е�ѹû�е���
				if(faprivatedata.FA_Problem)
				{
					status = 0;
				}
				else 
					status = 1;
				flag &= 0xfd;
				times = 0;
				LOG_INFO("Check_PowerOFF CT power Ia1 = %d, Ib1 = %d, Ic1 = %d;",(UInt32)ycvalue->Ia1.Param, (UInt32)ycvalue->Ib1.Param, (UInt32)ycvalue->Ic1.Param );
			}
			else
			{
				flag |= 0x02; 
				status = 0;
			}
		}
		else
		{
			if(faprivatedata.FA_Problem == 0)
				status = 1;
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
	UChar i;
	static UChar GLflag[3] = {0};
	static UChar GLstatus[3] = {0};
	static unsigned long GLtimes[3] = {0};
	static UInt32 GLRECOVERtimes[3] = {0};
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
				faprivatedata.FA_Problem = 1;
				GLtimes[num] = 0;
				LEDDATA &= ~(0x01<<LED_PROT);
				LED_SENDOUT(LEDDATA);
				//����ȷʵ���� ����soe cos
				yxstatus |= 0x01 << ALARM; 
				Send_CS(ALARM);
				//ע�� ��ʽ�汾Ӧ��ȥ��
				LOG_INFO("guoliu num = %d ,GLflag = %d , Ia1 = %d, Ib1 = %d, Ic1 = %d;", 
					num, GLflag[num], (UInt32)ycvalue->Ia1.Param,(UInt32)ycvalue->Ib1.Param , (UInt32)ycvalue->Ic1.Param );
				// ������բ
				if((faparm->faparm->softenable >> 10) & 0x01)
				{
					for(i=0;i<3;i++)
					{
						status = OpenDoor();
						if(status != 0)	
						{
							// ����,����ִ��
							continue;
						}
						break;
					}
					if(status != 0)
					{
						/* ��������ָʾ�� LED2 */
						LEDDATA &= ~(0x01<<LED_PROB);
						LED_SENDOUT(LEDDATA);
						LOG_INFO("open is err. ");
					}
					else
					{
						yxstatus |= 0x01 << ACTION; 
						Send_CS(ACTION);
					}
						

					//�澯 �¹���
					yxstatus |= 0x01 << ALLERR; 
					Send_CS(ALLERR);
				}
				status = 1;
			}
		}
		else
		{
			// ����ʱ��
			GLRECOVERtimes[num] = 0;
			//�غ�բ�������ܹ������ж��Ƿ����
			if(faprivatedata.old_fastatus == PROTECT)
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
			if(faprivatedata.old_fastatus == PROTECT)
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
				faprivatedata.FA_Problem = 0;
				GLRECOVERtimes[num] = 0;
				//����ʧȥ ����soe cos
				yxstatus &= ~(0x01 << ALARM);
				Send_CS(ALARM);
				//��������ȥ��
				Send_CS(ACTION);
				//�¹���ȥ��
				yxstatus &= ~(0x01 << ALLERR);
				Send_CS(ALLERR);
				//�غ�ȥ��
				if((faparm->faparm->softenable >> 13) & 0x01)
				{
					yxstatus &= ~(0x01 << RECLOSE);
					Send_CS(RECLOSE);	
				}
				if(faprivatedata.ReCounter && (faprivatedata.old_fastatus == PROTECT))
				{
					// ����غ�բ������¼
					faprivatedata.ReCounter = 0;
				}

				// Ϩ�����ָʾ�� LED2
				LEDDATA |= (0x01<<LED_PROT);
				LED_SENDOUT(LEDDATA);
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
	UChar i;
	static UChar LXGLflag[3] = {0};
	static UChar LXGLstatus[3] = {0};
	static unsigned long LXGLtimes[3] = {0};
	static UInt32 LXGLRECOVERtimes[3] = {0};
	
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
				faprivatedata.FA_Problem = 1;
				LEDDATA &= ~(0x01<<LED_PROT);
				LED_SENDOUT(LEDDATA);

				//����ȷʵ���� ����soe cos
				yxstatus |= 0x01 << ALARM;
				Send_CS(ALARM);
				//ע��  ���Գ���,��ʽ�汾ȥ��
				LOG_INFO("lxguoliu num = %d ,	LXGLflag = %d , I01 = %d", 
					num, LXGLflag[num], (UInt32)ycvalue->I01.Param);
				if((faparm->faparm->softenable >> 10) & 0x01)
				{
					for(i=0;i<3;i++)
					{
						status = OpenDoor();
						if(status != 0)	
						{
							// ����,����ִ��
							continue;
						}
						break;
					}
					if(status != 0)
					{
						/* ��������ָʾ�� LED2 */
						LEDDATA &= ~(0x01<<LED_PROB);
						LED_SENDOUT(LEDDATA);
						LOG_INFO("open is err. ");
						//��·������
					}
					else
					{
						yxstatus |= 0x01 << ACTION;
						Send_CS(ACTION);
					}
						
					//�澯 �¹���
					yxstatus |= 0x01 << ALLERR;
					Send_CS(ALLERR);
				}
				status = 1;
			}	
		}
		else
		{
			LXGLRECOVERtimes[num] = 0;
			if(faprivatedata.old_fastatus == PROTECT)
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
			if(faprivatedata.old_fastatus == PROTECT)
			{
				// ������ϱ�־
				LXGLstatus[num] = 0;
			}
			if(LXGLRECOVERtimes[num] == 0)
			{
				// ����ң�������ʱ
				LXGLRECOVERtimes[num] = tickets + faparm->faparm->problmeyx * 1000;
			}
			else if(LXGLRECOVERtimes[num] < tickets || FGflag)  //�����źŷ���ʱ����ֱ��������ϱ�־
			{
				//���������־
				LXGLstatus[num] = 0;
				LXGLRECOVERtimes[num] = 0;
				faprivatedata.FA_Problem = 0;
				//����ʧȥ ����soe cos�������
				yxstatus &= ~(0x01 << ALARM);
				Send_CS(ALARM);
				//��������ȥ��
				Send_CS(ACTION);
				//�¹���ȥ��
				yxstatus &= ~(0x01 << ALLERR);
				Send_CS(ALLERR);
				//�غ�ȥ��
				if((faparm->faparm->softenable >> 13) & 0x01)
				{
					yxstatus &= ~(0x01 << RECLOSE);
					Send_CS(RECLOSE);	
				}
				if(faprivatedata.ReCounter && (faprivatedata.old_fastatus == PROTECT))
				{
					// ����غ�բ������¼
					faprivatedata.ReCounter = 0;
				}
				// Ϩ�����ָʾ�� LED2
				LEDDATA |= (0x01<<LED_PROT);
				LED_SENDOUT(LEDDATA);
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
	UChar status = 0;
	UInt32 problem = 0;
	float *ycdata = (float *)(ShareRegionAddr.base_addr + faparm->faparm->ycover[num].ycindex *2);
	UInt32 yuxvalue = faparm->faparm->ycover[num].value;

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
	
	//Ĭ��1Ϊ����״̬
	problem = (yxstatus >> YUEXBASE) & 0x01;
	if(status)
	{
		if(problem == 0)
		{
			//����
			yxstatus |= 0x01 << YUEXBASE;
			Send_CS(YUEXBASE);
			value = 1;
			LOG_INFO("problem is yuex num = %d ,	ycdata = %d ,yuxvalue = %d ", 
				num,  (UInt32)ycdata[0], (UInt32)yuxvalue);
		}	
	}
	else
	{
		if(problem)
		{
			//ȡ��
			yxstatus &= ~(0x01 << YUEXBASE);
			Send_CS(YUEXBASE);
			LOG_INFO("recover is yuex num = %d ,	ycdata = %d ,yuxvalue = %d ", num,  (UInt32)ycdata[0], (UInt32)yuxvalue);
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
	SYSPARAMSTR *sysparm = (SYSPARAMSTR *)ShareRegionAddr.sysconf_addr;

	//���մ���
	if( yxstatus & (0x01 << (FWYX - 1)) )
	{
		// δ�������Ϊ�����ڵ� δ����ʱΪ�͵�ƽ//���մ���
		if((yxstatus >> (WCNYX -1)) & 0x01)
		{
			Task_sleep(faparm->faparm->chargetime);
		}
		// ִ���غ�բ����
		if((faparm->faparm->softenable >> 13) & 0x01)
		{
			switch(faprivatedata.ReCounter)
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
			faprivatedata.ReCounter++;
			yxstatus |= 0x01 << RECLOSE;
			Send_CS(RECLOSE);
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
//	UInt32 *yxstatus = (UInt32 *)ShareRegionAddr.digitIn_addr;
	UChar status = 0;
	faprivatedata.FA_Problem = 0;
	faprivatedata.ReCounter = 0;
	faprivatedata.old_fastatus = NOPOWER;

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
					faprivatedata.old_fastatus = NOPOWER;
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
						faprivatedata.old_fastatus = RUNNING;
						fastatus = 	PROTECT;
						break;
					}
				}
				// ����������
				for(i=0;i<ycvaluestr->faparm->zerosection_n;i++)
				{
					status = Check_LXGL(ycvaluestr, remotevalue, i);
					if(status && ((ycvaluestr->faparm->softenable >> 13) & 0x01))
					{
						faprivatedata.old_fastatus = RUNNING;
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
					faprivatedata.old_fastatus = RUNNING;
					fastatus = 	NOPOWER;
					LOG_INFO("Change status to NOPOWER! ;");
				}
				break;
			case PROTECT:
				if(faprivatedata.ReCounter < ycvaluestr->faparm->reclose_n)
				{
					status = Reclose_State(ycvaluestr);
					if(status)
					{
						faprivatedata.old_fastatus = PROTECT;
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
					faprivatedata.ReCounter = 0;
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
/***************************************************************************/
//����:	void Send_CS(UChar num)
//˵��:	����SOE��COS����,Ĭ��0ΪĬ��״̬,1Ϊʹ�ܹ���״̬
//����: num ������ң�ű��
//���: ��
//�༭:
//ʱ��:2015.8.17
/***************************************************************************/
void Send_CS(UChar num)
{
	YCValueStr *faparm = &ycvalueprt;
	UInt32 channel = 0;
	UInt32 *yxdata = (UInt32 *)ShareRegionAddr.digitIn_addr;
	UChar status = 0;

	//�ı�ͨ��
	channel = 0x01 << num;	
	//�ı���״̬
	status = (yxstatus >> num) & 0x01;
	
	if(((faparm->faparm->softenable >> 17 ) & 0x01)&&((dianbiaodata.yxcos >> num ) & 0x01))
	{
		// ȡ��λ�Ƿ�ʹ��
		if((dianbiaodata.yxnot >> num) & 0x01)
		{
			if(status)
			{
				yxdata[0] &= ~(0x01 << num);
			}
			else
			{
				yxdata[0] |= 0x01 << num;
			}
		}
		else
		{
			if(status)
			{
				yxdata[0] |= 0x01 << num;
			}
			else
			{
				yxdata[0] &= ~(0x01 << num);
			}			
		}
		Message_Send(MSG_COS, channel, yxdata[0]);
	}
	else
	{
		//����û��ʹ��SOE��COS����Ȼ��Ҫ���ɼ�ң������ͬ����������
		if(status)
		{
			yxdata[0] |= 0x01 << num;
		}
		else
		{
			yxdata[0] &= ~(0x01 << num);
		}
	}
	if(((faparm->faparm->softenable >> 18 ) & 0x01)&&((dianbiaodata.yxsoe >> num) & 0x01))
	{
		Message_Send(MSG_SOE, channel, yxdata[0]);				
	}
	LOG_INFO("Send_CS num %d, status %d",num,status);
}








