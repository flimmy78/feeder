/*************************************************************************/
/*
 * Copyright (c) 2015, ֣�����ܵ��� Runner
 * All rights reserved.
 * file       : fft.c
 * Design     : ZLB
 * Data       : 2015-4-27
 * Version    : V1.0
 * Change     :
 */
/*************************************************************************/
/* xdctools header files */
#include <stdlib.h>
#include <string.h>
#include <xdc/std.h>
#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

#include <ti/sysbios/timers/timer64/Timer.h>
#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Queue.h>
#include <ti/sysbios/knl/Event.h>

#include "mathlib.h" 								//DSP��ѧ������
#include "dsplib.h"                 				//DSP������

#include "queue.h"
#include "communicate.h"
#include "sht11.h"
#include "fft.h"
#include "sys.h"
#include "log.h"
#include "fa.h"
#include "ad7606.h"


/****************************** extern define ************************************/


/******************************* private data ***********************************/
//FFT��ת��������
#pragma DATA_ALIGN(Cw, 8);
float Cw[2*TN] =
{
   0.7071068, 0.7071068,       0.0,       1.0, 0.9238796,  0.3826835, 0.7071068, 0.7071068,
   0.3826835, 0.9238796,       0.0,       1.0, 0.9807853,  0.1950903, 0.9238796, 0.3826835,
   0.8314696, 0.5555702, 0.7071068, 0.7071068, 0.5555702,  0.8314697, 0.3826835, 0.9238796,
   0.1950902, 0.9807853,       0.0,       1.0, 0.9951848, 0.09801714, 0.9807853, 0.1950903,
   0.9569404, 0.2902847, 0.9238796, 0.3826835, 0.8819213,  0.4713968, 0.8314696, 0.5555702,
   0.7730104, 0.6343933, 0.7071068, 0.7071068, 0.6343933,  0.7730104, 0.5555702, 0.8314697,
   0.4713967, 0.8819213, 0.3826835, 0.9238796, 0.2902847,  0.9569404, 0.1950902, 0.9807853,
   0.09801713, 0.9951848,       0.0,       1.0,       0.0,        0.0,       0.0,       0.0
};
// ������λ��ת
#pragma DATA_ALIGN (brev, 8);
unsigned char brev[64]=
{
	0x0, 0x20, 0x10, 0x30, 0x8, 0x28, 0x18, 0x38,
	0x4, 0x24, 0x14, 0x34, 0xc, 0x2c, 0x1c, 0x3c,
	0x2, 0x22, 0x12, 0x32, 0xa, 0x2a, 0x1a, 0x3a,
	0x6, 0x26, 0x16, 0x36, 0xe, 0x2e, 0x1e, 0x3e,
	0x1, 0x21, 0x11, 0x31, 0x9, 0x29, 0x19, 0x39,
	0x5, 0x25, 0x15, 0x35, 0xd, 0x2d, 0x1d, 0x3d,
	0x3, 0x23, 0x13, 0x33, 0xb, 0x2b, 0x1b, 0x3b,
	0x7, 0x27, 0x17, 0x37, 0xf, 0x2f, 0x1f, 0x3f
};

static float fft_in_buff[8][64];				//�������� ����ң��ADֵ

BaseValueType BaseFFToutValue[8];				//�����ݻ��� 1*8 1��ADоƬ 8 ��ͨ�� ģֵ ���
BaseFFTout FFTBaseValue[8];					//fft ����ֵ(һ��г��)
float AdjustValue[8];							//У׼ֵ���飬һ��ͨ��һ��У׼ֵ
static float FFT_Out[2*TN];						//���һ��ͨ����FFTת�����
Queue *faqueue;									//fft��faͨ�Ŷ���ָ��						
Semaphore_Handle fasem;							//FFT��FA����ͨ���ź���
YCValueStr ycvalueprt;							//ң��ṹ��
/******************************* functions **************************************/


/***************************************************************************/
//����:	static Void Copy_2_FFTIn(AD7606Str *ad, channalstr *output)
//˵��:	ad ad�ɼ�ģ��
//����: ��
//���: ��
//�༭:
//ʱ��:2015.4.24
/***************************************************************************/
static void ad7606_tick(UArg ptr)
{
	/*ad7606_start*/
#if SETAD7606_1
	SetIO_HIGH(AD7606_CLK_AD1);
#else
	SetIO_HIGH(AD7606_CLK_AD2);
#endif
}
/***************************************************************************/
//����:	static Void Copy_2_FFTIn(AD7606Str *ad, channalstr *output)
//˵��:	ad ad�ɼ�ģ��
//����: ��
//���: ��
//�༭:
//ʱ��:2015.4.24
/***************************************************************************/
Void NewTimerInit(AD7606Str *handle, CurrentPaStr *freq)
{
	UInt16 period;
	Error_Block eb;
    Error_init(&eb);

	Timer_Params timerParams;
	Timer_Params_init(&timerParams);

	// ���ö�ʱ������ 1000000/(freq/100)/64 freq�Ѿ�������100��
	period = (UInt16)(1000000/(freq->F1.Param * 64));
	// ������ʱ��
    timerParams.period = period;
    timerParams.periodType = Timer_PeriodType_MICROSECS;
	timerParams.startMode = Timer_StartMode_USER;
	/* ������ʱ��2 */
	handle->clock = Timer_create(2, ad7606_tick, &timerParams, &eb);

	if(handle->clock == NULL)
	{
		LOG_INFO("NewTimerInit is err;");
	}
	LOG_INFO("NewTimerInit is OK, period is %d; ",period);
}
/***************************************************************************/
//����:	static Void Copy_2_FFTIn(AD7606Str *ad, channalstr *output)
//˵��:	ad ad�ɼ�ģ��
//����: ��
//���: ��
//�༭:
//ʱ��:2015.4.24
/***************************************************************************/
static Void Copy_2_FFTIn(AD7606Str *ad, channalstr *output)
{
	UChar i;
	channalstr *buff = ad->databuff;
	UChar flag = ad->flagfreq & 0x01;
	
	/* ����ad�ɼ����� flag */
	if(flag)
	{
		for(i=0;i<8;i++)
		{
			/* ��λ���� ��32��������λ��ǰ��*/
			memcpy((char *)&(output[i].channel[0]), (char *)fft_in_buff[i], TN*4);
			memcpy((char *)&(output[i].channel[64]), (char *)&(buff[i].channel[0]), TN*4);
			memcpy((char *)fft_in_buff[i], (char *)&(buff[i].channel[0]), TN*4);
		}
	}
	else
	{
		for(i=0;i<8;i++)
		{
			/* ��λ���� ��32��������λ��ǰ��*/
			memcpy((char *)&(output[i].channel[0]), (char *)fft_in_buff[i], TN*4);
			memcpy((char *)&(output[i].channel[64]), (char *)&(buff[i].channel[64]), TN*4);
			memcpy((char *)fft_in_buff[i], (char *)&(buff[i].channel[64]), TN*4);
		}
	}
}
/***************************************************************************/
//����:	void tw_gen(float *w, int n)
//˵��:	������ת����
//����: w �����ת�������� n��������
//���: ��
//�༭:
//ʱ��:2015.4.29
/***************************************************************************/
static void tw_gen(float *w, int n)
{
	int i,j,k;
	double x_t,y_t,theta1,theta2,theta3;

	for(j=1,k=0;j<=n>>2;j=j<<2)
	{
		for(i=0;i<n>>2;i+=j)
		{
			theta1=2*PI*i/n;
			x_t=cossp(theta1);
			y_t=sinsp(theta1);
			w[k]=(float)x_t;
			w[k+1]=(float)y_t;

			theta2=4*PI*i/n;
			x_t=cossp(theta2);
			y_t=sinsp(theta2);
			w[k+2]=(float)x_t;
			w[k+3]=(float)y_t;

			theta3=6*PI*i/n;
			x_t=cossp(theta3);
			y_t=sinsp(theta3);
			w[k+4]=(float)x_t;
			w[k+5]=(float)y_t;
			k+=6;
		}
	}
}
/***************************************************************************/
//����:	Void GetTempture(CurrentPaStr *remotevalue)
//˵��:	��ʪ�Ȳɼ� �¶Ⱦ���14bit ʪ�Ⱦ���12bit
//����: remotevalue ң�����ݽṹ��
//���: ��
//�༭:
//ʱ��:2015.8.17
/***************************************************************************/
Void GetTempture(CurrentPaStr *remotevalue)
{
	UInt16 datatmp = 0;

	datatmp = Measure_Data(TEMP_CMD, TEMP_DELAY_14);
	remotevalue->DC2.Param = Convert_Temp(datatmp);
	datatmp = Measure_Data(HUMI_CMD, TEMP_DELAY_12);
	remotevalue->H1.Param = Convert_Humi(datatmp, remotevalue->DC2.Param);
}
/***************************************************************************/
//����:	Void FFT_Task(UArg arg0, UArg arg1) 
//˵��:	���ݼ�������
//����: arg0 arg1
//���: ��
//�༭:
//ʱ��:2015.4.27
/***************************************************************************/
static Void PowerValue(YCValueStr *basevalue, CurrentPaStr *remotevalue)
{
	UChar i;
//	UInt32 tmpx = 0;
//	UInt32 tmpy = 0;
	float x = 0;
	float y = 0;

	//�����Чֵ ����adjust����У׼ͨ����ѹ 
	for(i=0;i<8;i++)
	{
		if(i==7)
		{
			/* DCͨ��,ֱ������/64 */
			basevalue->modeangel[i].module = (basevalue->adjust[i]*basevalue->fftout[i].real)/64;
		}
		else
		{
			basevalue->modeangel[i].module = sqrtsp(basevalue->fftout[i].real*basevalue->fftout[i].real + 
				basevalue->fftout[i].image*basevalue->fftout[i].image);
			//57.29577=180/pi
			basevalue->modeangel[i].angle = atan2sp(basevalue->fftout[i].image,basevalue->fftout[i].real)*57.29577;
		}
	}
	
	/* ������ */
	remotevalue->Ua1.Param = basevalue->modeangel[0].module;// * sysparm->ptrate ;
	/* �жϹ������ѹ�Ƿ�ʧ�� ʧ������Ҫ�л� �˴�Ϊ�ָ�Ϊ�����ⲿ��Ϊ����Ƶ�� */
	if(remotevalue->Ua1.Param> basevalue->faparm->lowvol)
	{
		/* �����ʽΪ1 �����ڲ��������Ƶ�� */
		if(ad7606Str.flagfreq & (0x01 << 2))
		{
			ad7606Str.flagfreq &= ~(0x01<<2);
			ad7606Str.flagfreq |= 0x01<<1;
			/* ʹ���л� */
#if SETAD7606_1
			SetIO_LOW(AD7606_SELCLK_AD1);
#else
			SetIO_LOW(AD7606_SELCLK_AD2);
#endif
			Timer_stop(ad7606Str.clock);
		}
	}
	remotevalue->Ub1.Param = basevalue->modeangel[1].module;// * sysparm->ptrate;
	remotevalue->Uc1.Param = basevalue->modeangel[2].module;// * sysparm->ptrate;
	remotevalue->Ia1.Param = basevalue->modeangel[3].module;// * sysparm->ctrate;
	remotevalue->Ib1.Param = basevalue->modeangel[4].module;// * sysparm->ctrate;
	remotevalue->Ic1.Param = basevalue->modeangel[5].module;// * sysparm->ctrate;
	remotevalue->I01.Param = basevalue->modeangel[6].module;// * sysparm->ctrate;
	remotevalue->DC1.Param = basevalue->modeangel[7].module;// DC value

	//����ɼ�����ΪA��C������ʱ,����B�����
	x = basevalue->fftout[6].real - basevalue->fftout[3].real - basevalue->fftout[5].real; 
	y = basevalue->fftout[6].image - basevalue->fftout[3].image - basevalue->fftout[5].image;
	remotevalue->Ib1.Param = sqrtsp(x*x + y*y);
	//����ɼ���ѹΪA��Cʱ,�����й��������޹�����
	remotevalue->P1.Param = P_Value(0,1);
	remotevalue->Q1.Param = Q_Value(0,1);
	
	return ;

	/* �й� P1 = Ur*Ir+Ui*Ii */
	remotevalue->Pa1.Param = basevalue->fftout[0].real*basevalue->fftout[3].real + 
		basevalue->fftout[0].image*basevalue->fftout[3].image;
	remotevalue->Pb1.Param = basevalue->fftout[1].real*basevalue->fftout[4].real + 
		basevalue->fftout[1].image*basevalue->fftout[4].image;
	remotevalue->Pc1.Param = basevalue->fftout[2].real*basevalue->fftout[5].real + 
		basevalue->fftout[2].image*basevalue->fftout[5].image;
	/* ���й� */
	remotevalue->P1.Param = remotevalue->Pa1.Param + remotevalue->Pb1.Param + remotevalue->Pc1.Param;

	/* �޹� Q = Ui*Ir-Ur*Ii */
	remotevalue->Qa1.Param = basevalue->fftout[0].image*basevalue->fftout[3].real - 
		basevalue->fftout[0].real*basevalue->fftout[3].image;
	remotevalue->Qb1.Param = basevalue->fftout[1].image*basevalue->fftout[4].real - 
		basevalue->fftout[1].real*basevalue->fftout[4].image;
	remotevalue->Qc1.Param = basevalue->fftout[2].image*basevalue->fftout[5].real - 
		basevalue->fftout[2].real*basevalue->fftout[5].image;
	/* ���޹� */
	remotevalue->Q1.Param = remotevalue->Qa1.Param+remotevalue->Qb1.Param+remotevalue->Qc1.Param;
	/* ��������Cos1 P/S S���ڹ��� S = sqrtsp(P1*P1+Q1*Q1)*/
	remotevalue->Cos1.Param = remotevalue->P1.Param/sqrtsp(remotevalue->P1.Param*remotevalue->P1.Param+ remotevalue->Q1.Param*remotevalue->Q1.Param);
	// �����ѹ
	x = basevalue->fftout[0].real + basevalue->fftout[1].real + basevalue->fftout[2].real;
	y = basevalue->fftout[0].image + basevalue->fftout[1].image + basevalue->fftout[2].image;
	
	remotevalue->U01.Param =sqrtsp(x*x + y*y);
/*	
	tmpx = basevalue->modeangel[0].module * sinsp(basevalue->modeangel[0].angle*PI_180) +
						  basevalue->modeangel[1].module * sinsp(basevalue->modeangel[1].angle*PI_180) +
						  basevalue->modeangel[2].module * sinsp(basevalue->modeangel[2].angle*PI_180);
	tmpy = basevalue->modeangel[0].module * cossp(basevalue->modeangel[0].angle*PI_180) +
					  basevalue->modeangel[1].module * cossp(basevalue->modeangel[1].angle*PI_180) +
					  basevalue->modeangel[2].module * cossp(basevalue->modeangel[2].angle*PI_180);

	remotevalue->U01.Param = 1000*sqrtsp(tmpx*tmpx + tmpy*tmpy);   */
	//�������
	x = basevalue->fftout[3].real + basevalue->fftout[4].real + basevalue->fftout[5].real;
	y = basevalue->fftout[3].image + basevalue->fftout[4].image + basevalue->fftout[5].image;
	
	remotevalue->I01.Param =sqrtsp(x*x + y*y);
/*	
	tmpx = basevalue->modeangel[3].module * sinsp(basevalue->modeangel[3].angle*PI_180) +
						  basevalue->modeangel[4].module * sinsp(basevalue->modeangel[4].angle*PI_180) +
						  basevalue->modeangel[5].module * sinsp(basevalue->modeangel[5].angle*PI_180);
	tmpy = basevalue->modeangel[3].module * cossp(basevalue->modeangel[3].angle*PI_180) +
					  basevalue->modeangel[4].module * cossp(basevalue->modeangel[4].angle*PI_180) +
					  basevalue->modeangel[5].module * cossp(basevalue->modeangel[5].angle*PI_180);

	remotevalue->I01.Param = 1000*sqrtsp(tmpx*tmpx + tmpy*tmpy);  */

	
}
// �й�����
float P_Value(UInt8 u1_ch, UInt8 u2_ch)
{
	float ptemp = 0;

	ptemp = ycvalueprt->fftout[u1_ch].real*ycvalueprt->fftout[u1_ch+3].real + ycvalueprt->fftout[u1_ch].image*ycvalueprt->fftout[u1_ch+3].image;
	ptemp = ptemp + ycvalueprt->fftout[u2_ch].real*ycvalueprt->fftout[u2_ch+3].real + ycvalueprt->fftout[u2_ch].image*ycvalueprt->fftout[u2_ch+3].image;
	return ptemp;
}
// �޹�����
float Q_Value(UInt8 u1_ch, UInt8 u2_ch)
{
	float ptemp = 0;

	ptemp = ycvalueprt->fftout[u1_ch].image*ycvalueprt->fftout[u1_ch+3].real - ycvalueprt->fftout[u1_ch].real*ycvalueprt->fftout[u1_ch+3].image;
	ptemp = ptemp + ycvalueprt->fftout[u2_ch].image*ycvalueprt->fftout[u2_ch+3].real - ycvalueprt->fftout[u2_ch].real*ycvalueprt->fftout[u2_ch+3].image;
	return ptemp;
}
/***************************************************************************/
//����:	Void FFT_Task(UArg arg0, UArg arg1) 
//˵��:	���ݼ�������
//����: arg0 arg1
//���: ��
//�༭:
//ʱ��:2015.4.27
/***************************************************************************/
Void FFT_Task(UArg arg0, UArg arg1) 
{
	UChar i;
	Bool status = 0;
	CurrentPaStr *remotevalue = (CurrentPaStr *)ShareRegionAddr.base_addr;
	/* Ĭ�Ϲ�ƵΪ50hz ARM�ϴ�ʱ��Ҫ/100 */
	remotevalue->F1.Param = 50.00;
	ycvalueprt.fftout = FFTBaseValue;
	ycvalueprt.modeangel = BaseFFToutValue;
	ycvalueprt.adjust = AdjustValue;
	ycvalueprt.faparm = (FAPARAMSTR *)ShareRegionAddr.faconf_addr;
	
	//��ʼ����ת����
	tw_gen(Cw,TN);	
	//ʹ��Ƶ�ʼ���ʱ��		    
	Clock_Init(remotevalue);
	//Timer_Init(remotevalue);
	StartClock(&ad7606Str);

	//fasem = Semaphore_create(0, NULL, NULL);
	LOG_INFO("FFT_Task Init is OK;");
	
	while(1)
	{
		/* �ȴ�ң�����ݸ��� �ȴ�ʱ��Ϊ20ms  ���໷Ĭ��Ƶ��678hz �ɼ�32������Ҫ47ms*/
		status = Semaphore_pend(ad7606Str.sem, 20);
		if(status == 0)
		{
			/* �ж��Ƿ��Ѵ���timer */
			if(((ad7606Str.flagfreq >> 3) & 0x01)  == 0)
			{
				/* ���໷δ���Ƶ�� */	
				NewTimerInit(&ad7606Str, remotevalue);
				ad7606Str.flagfreq |= 0x01<<3;
			}
			/* �л������ʽ */
			ad7606Str.flagfreq |= 0x01<<2;
			/* ֹͣƵ�ʲɼ� */
			ad7606Str.flagfreq &= ~(0x01<<1);
			remotevalue->F1.Param = 0;
			
			/* ʹ���л� */
#if SETAD7606_1
			SetIO_HIGH(AD7606_SELCLK_AD1);
#else
			SetIO_HIGH(AD7606_SELCLK_AD2);
#endif
			Timer_start(ad7606Str.clock);
			LOG_INFO("Semaphore_pend over 20ms,change clock output;");
		}		
		Copy_2_FFTIn(&ad7606Str, FFT_In);	
		
		/* ����ͨ���ɼ����� */ /* ������FFT֮ǰ���FIR�˲� */
		for(i=0;i<8;i++)
		{
			/* i=7 Ϊֱ������ �����ƽ��ֵ*/
			if(i == 7 )
			{
				ycvalueprt.fftout[i].real = SumDC((float *)&FFT_In[i],2*TN);
			}
			else
			{
				// dsp fft�������
				DSPF_sp_fftSPxSP(TN,(float *)&FFT_In[i],Cw,FFT_Out,brev,4,0,TN);
				
				ycvalueprt.fftout[i].real = FFT_Out[2]*sqrt2*ycvalueprt.adjust[i]/64;
				ycvalueprt.fftout[i].image = FFT_Out[3]*sqrt2*ycvalueprt.adjust[i]/64;
			}
		}
		PowerValue(&ycvalueprt, remotevalue);	
		// ����ר��GPIO_PIN_HIGH
//		LOG_INFO("freq is %d status is %d,remotevalue->Ua1.Param is %d;remotevalue->Ub1.Param is %d;remotevalue->Uc1.Param is %d;remotevalue->U01.Param is %d;remotevalue->Ia1.Param is %d; remotevalue->Ib1.Param is %d;remotevalue->Ic1.Param is %d;remotevalue->Cos1.Param is %d",
//				(UInt32)remotevalue->F1.Param,status,(UInt32)remotevalue->Ua1.Param,(UInt32)remotevalue->Ub1.Param,(UInt32)remotevalue->Uc1.Param,(UInt32)remotevalue->U01.Param,(UInt32)remotevalue->Ia1.Param,(UInt32)remotevalue->Ib1.Param,(UInt32)remotevalue->Ic1.Param,(UInt32)remotevalue->Cos1.Param);
		// ����fa�����ź��� ִ��fa�ж�
		Semaphore_post(fasem); 		//2015-10-30 change
	}
}/***************************************************************************/
//����:	float SumDC(float *data, Int8 len)
//˵��:	�������
//����: data ����ָ�� len ���ݳ���
//���: ���ݺ�
//�༭:
//ʱ��:2015.4.27
/***************************************************************************/
float SumDC(float *data, Int8 len)
{
	Int8 i;
	float sum = 0;

	for(i=0;i<len;i++)
	{
		sum += data[i];
	}
}


