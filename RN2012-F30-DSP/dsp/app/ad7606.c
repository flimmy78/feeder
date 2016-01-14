/*************************************************************************/
/*
 * Copyright (c) 2015, ֣�����ܵ��� Runner All rights reserved.
 *
 * file       : ad7606.c
 * Design     : zlb
 * Email	  : longbiao831@qq.com
 * Data       : 2015-7-28
 * Version    : V1.0
 * Change     :	2015-10-14  �޸Ķ�ȡ��������ʱ���� ��245�д���ӿն�һ�����ݵĳ��������ݴ�λ������
 * Description: AD7606���ݲɼ����ֳ���ʵ��
 */
/*************************************************************************/
#include <stdlib.h>
#include <string.h>

#include "ad7606.h"

#include <xdc/std.h>

#include <xdc/runtime/Error.h>
#include <xdc/runtime/System.h>

#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/hal/Hwi.h>

#include <ti/sysbios/knl/Task.h>
#include <ti/sysbios/knl/Semaphore.h>
#include <ti/sysbios/knl/Mailbox.h>
#include <ti/sysbios/knl/Clock.h>

#include "hw/hw_types.h"
#include "hw/hw_syscfg0_OMAPL138.h"
#include "hw/hw_psc_OMAPL138.h"
#include "hw/soc_OMAPL138.h"
#include "c674x/omapl138/interrupt.h"
#include "EMIFAPinmuxSetup.h"
#include "gpio.h"
#include "psc.h"
#include "emifa.h"
#include "sys.h"
#include "log.h"
#include "fft.h"



/* interrupt bank */
#define AD7606_AD1_BUSY_GPIO_BANK	 		2
/* event id */
#define AD7606_AD1_BUSY_GPIO_BANK_INT	SYS_INT_GPIO_B2INT
/* �������жϺ� */
#define C674X_MASK_INT4  					4		//�ж�������
/* AD������ѹ��Χ */
#define MAX_RANGE							10		//AD�Ĳ�����ΧΪ-10V~+10V

static void AD7606_BUSY_Hwi(UArg ad_addr);
/***************************************************************************/
//����:static void ad7606_emifa_init(void)
//˵��:��ʼ��ad7606emifa����	
//����: ��
//���: 
//�༭:zlb
//ʱ��:2015.7.28
/***************************************************************************/
static void EMIFA_AD7606_Init(void)
{
	/* enable emifa mode */
	PSCModuleControl(SOC_PSC_0_REGS, HW_PSC_EMIFA, PSC_POWERDOMAIN_ALWAYS_ON,
			PSC_MDCTL_NEXT_ENABLE);
	/* enable gpio psc */
	PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_GPIO, PSC_POWERDOMAIN_ALWAYS_ON,
			PSC_MDCTL_NEXT_ENABLE);
	
	/*selects the EMIFA pins for use*/
	EMIFAPinMuxSetup();

	/* CS2 set AD7606 1 */
	/*set the buswidth of async device connected.  16bit*/
	EMIFAAsyncDevDataBusWidthSelect(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_2,
					EMIFA_DATA_BUSWITTH_16BIT);

	/*selects the aync interface opmode. :Normal Mode*/
	EMIFAAsyncDevOpModeSelect(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_2,
	EMIFA_ASYNC_INTERFACE_NORMAL_MODE);

	/*Extended Wait disable.*/
	EMIFAExtendedWaitConfig(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_2,
	EMIFA_EXTENDED_WAIT_DISABLE);

	/*configures the wait timing for the device interfaced on CS2
	* W_SETUP/W_HOLD W_STROBE/R_STROBE W_HOLD/R_HOLD TA*/
//	EMIFAWaitTimingConfig(SOC_EMIFA_0_REGS, EMIFA_CHIP_SELECT_2,
//                          EMIFA_ASYNC_WAITTIME_CONFIG(1, 2, 1, 1, 3, 1, 1));	
//							EMIFA_ASYNC_WAITTIME_CONFIG(1, 2, 1, 2, 3, 2, 0));
	HWREG(SOC_EMIFA_0_REGS + EMIFA_CE2CFG) |= EMIFA_ASYNC_WAITTIME_CONFIG(1, 2, 1, 2, 4, 2, 0);

//	/* CS4 set AD7606 2 */
//	/*set the buswidth of async device connected.  16bit*/
//	EMIFAAsyncDevDataBusWidthSelect(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_4,
//					EMIFA_DATA_BUSWITTH_16BIT);
//	/*selects the aync interface opmode. :Normal Mode*/
//	EMIFAAsyncDevOpModeSelect(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_4,
//	EMIFA_ASYNC_INTERFACE_NORMAL_MODE);
//	/*Extended Wait disable.*/
//	EMIFAExtendedWaitConfig(SOC_EMIFA_0_REGS,EMIFA_CHIP_SELECT_4,
//	EMIFA_EXTENDED_WAIT_DISABLE);
//	/*configures the wait timing for the device interfaced on CS4
//	* W_SETUP/W_HOLD W_STROBE/R_STROBE W_HOLD/R_HOLD TA*/
//	EMIFAWaitTimingConfig(SOC_EMIFA_0_REGS, EMIFA_CHIP_SELECT_4,
//                          EMIFA_ASYNC_WAITTIME_CONFIG(1, 2, 1, 1, 3, 1, 1));
//							EMIFA_ASYNC_WAITTIME_CONFIG(1, 2, 1, 2, 3, 2, 0));
}
/***************************************************************************/
//����:static void AD7606_GPIO_Config(void)
//˵��:��ʼ��ad7606 gpio IO����	
//����:��
//���: 
//�༭:zlb
//ʱ��:2015.7.28
/***************************************************************************/
static void AD7606_GPIO_Config(void)
{
	volatile unsigned int savePinMux = 0;

	savePinMux = HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(5)) &
		~(SYSCFG_PINMUX5_PINMUX5_19_16 | SYSCFG_PINMUX5_PINMUX5_11_8 |
				SYSCFG_PINMUX5_PINMUX5_3_0 | SYSCFG_PINMUX5_PINMUX5_7_4);

	/* config busy and clk for ad1 */
	HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(5)) =
		(PINMUX5_AD2_BUSY_ENABLE | PINMUX5_AD1_BUSY_ENABLE |
				PINMUX5_AD1_CLK_ENABLE | PINMUX5_AD2_CLK_ENABLE | savePinMux);

	savePinMux = HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1)) &
		~(SYSCFG_PINMUX1_PINMUX1_11_8 | SYSCFG_PINMUX1_PINMUX1_7_4);

	/* config setclck for ad1 and ad2 */
	HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1)) =
		(PINMUX1_AD1_SELCLK_ENABLE | PINMUX1_AD2_SELCLK_ENABLE | savePinMux);

	savePinMux = HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(13)) &
		~(SYSCFG_PINMUX13_PINMUX13_31_28 | SYSCFG_PINMUX13_PINMUX13_27_24);

	/* get conver frequency for ad1 and ad2 */
	HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(13)) =
		(PINMUX13_AD1_CONVER_ENABLE | PINMUX13_AD2_CONVER_ENABLE | savePinMux);

}
/***************************************************************************/
//����:static void AD7606_GPIO_Setup(void)
//˵��:��ʼ��AD7606 GPIO����
//����:��
//���: 
//�༭:zlb
//ʱ��:2015.7.28
/***************************************************************************/
static void  AD7606_GPIO_Setup(void) 
{
	/*Set the AD7606 I/O direction */
	GPIODirModeSet(SOC_GPIO_0_REGS, AD7606_BUSY_AD1, GPIO_DIR_INPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, AD7606_CLK_AD1, GPIO_DIR_OUTPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, AD7606_SELCLK_AD1, GPIO_DIR_OUTPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, AD7606_CONVST_AD1, GPIO_DIR_OUTPUT);
	/* ����AD2 ��ֹ��ƽδ֪ */
	GPIODirModeSet(SOC_GPIO_0_REGS, AD7606_BUSY_AD2, GPIO_DIR_INPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, AD7606_CLK_AD2, GPIO_DIR_OUTPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, AD7606_SELCLK_AD2, GPIO_DIR_OUTPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, AD7606_CONVST_AD2, GPIO_DIR_OUTPUT);

	/*Set the AD7606_BUSY to the Falling Edge Interrupt*/
#ifdef SETAD7606_1
	GPIOIntTypeSet(SOC_GPIO_0_REGS, AD7606_BUSY_AD1, GPIO_INT_TYPE_FALLEDGE);
#else
	GPIOIntTypeSet(SOC_GPIO_0_REGS, AD7606_BUSY_AD2, GPIO_INT_TYPE_FALLEDGE);
#endif

	/* �����Ҫ��չ����������¼��жϵķ�ʽ */
	/*The interrupt setting allows AD7606_BUSY_GPIO_BANK*/
	GPIOBankIntEnable(SOC_GPIO_0_REGS, SYS_INT_GPIO_B2INT);

	/* set selclk is low,so default set convst clk 64 */
//	GPIOPinWrite(SOC_GPIO_0_REGS, AD7606_SELCLK_AD1, GPIO_PIN_HIGH);		//enable sys out clk
	GPIOPinWrite(SOC_GPIO_0_REGS, AD7606_SELCLK_AD1, GPIO_PIN_LOW);			//�͵�ƽѡ��Ĭ�����໷���� �ߵ�ƽdsp����ת��ʱ��
	GPIOPinWrite(SOC_GPIO_0_REGS, AD7606_SELCLK_AD2, GPIO_PIN_LOW);			//�͵�ƽѡ��Ĭ�����໷���� �ߵ�ƽdsp����ת��ʱ��
	GPIOPinWrite(SOC_GPIO_0_REGS, AD7606_CLK_AD1, GPIO_PIN_LOW);
	GPIOPinWrite(SOC_GPIO_0_REGS, AD7606_CLK_AD2, GPIO_PIN_LOW);
	GPIOPinWrite(SOC_GPIO_0_REGS, AD7606_CONVST_AD1, GPIO_PIN_HIGH);
	GPIOPinWrite(SOC_GPIO_0_REGS, AD7606_CONVST_AD2, GPIO_PIN_HIGH);
}
/***************************************************************************/
//����:static void AD7606_IRQ_Setup(AD7606Str *adptr)
//˵��:��ʼ��AD7606�ж� HWI�жϷ�ʽ
//����:adptr ad7606�ṹ��
//���:��
//�༭:zlb
//ʱ��:2015.7.31
/***************************************************************************/
static void AD7606_IRQ_Setup(AD7606Str *adptr)
{
	Hwi_Params hwiParams;
	Hwi_Handle myHwi;
	Error_Block eb;	
	
	Error_init(&eb);
	Hwi_Params_init(&hwiParams);
	hwiParams.arg = (UInt32)adptr;		//�жϴ�����
	hwiParams.enableInt = FALSE;
	hwiParams.eventId = adptr->event_id;
//	hwiParams.priority = 4;				//priority ���Բ�������

	myHwi = Hwi_create(adptr->irq_event, AD7606_BUSY_Hwi, &hwiParams, &eb);

	if(myHwi == NULL)
	{
		LOG_INFO("AD7606_IRQ_Setup failed to create hwi. ");
	}

	/* enable both interrupts */
	Hwi_enableInterrupt(adptr->irq_event);
	
}
/***************************************************************************/
//����:static void AD7606_IRQ_Setup(AD7606Str *adptr)
//˵��:��ʼ��AD7606�ж� HWI�жϷ�ʽ
//����:adptr ad7606�ṹ��
//���:��
//�༭:zlb
//ʱ��:2015.7.31
/***************************************************************************/
static void AD7606_BUSY_Hwi(UArg ad_addr)
{
	Int16 advalue = 0;
	UChar i;
	AD7606Str *ad = (AD7606Str *)ad_addr;

#ifdef SETAD7606_1
	if(GPIOPinIntStatus(SOC_GPIO_0_REGS, AD7606_BUSY_AD1) == GPIO_INT_PEND)
	{
		//��ȡ���ݣ���¼�������Ƿ�ﵽ32�����ݣ��ﵽʱ�����ź���
		for(i=0;i<8;i++)
		{
			// get original data 
		    advalue = ((Int16 *)SOC_EMIFA_CS2_ADDR)[i];
			// conversion data 
			ad->databuff[i].channel[ad->conter] = advalue;
			ad->databuff[i].channel[ad->conter+1] = 0.0;
		}
		ad->conter += 2;
		if(ad->conter > 63 & ((ad->flagfreq & 0x01) == 0))
		{
			ad->flagfreq |= 0x01;
			Semaphore_post(ad->sem);
		}
		else if(ad->conter > 127)
		{
			ad->flagfreq &= ~0x01;
			ad->conter = 0;
			Semaphore_post(ad->sem);
		}
		// Ƶ�ʲɼ� 
		if(ad->flagfreq & 0x02)
		{
			ad->freq++;
		}
	}

	/* Set low convst gpio to prepare the next conversion */
	if(ad->flagfreq & (0x01 << 2))
	{
		GPIOPinWrite(SOC_GPIO_0_REGS, AD7606_CLK_AD1, GPIO_PIN_LOW);
	}
	/* Clears the Interrupt Status of AD7606_BUSY_AD1 in GPIO.*/
	GPIOPinIntClear(SOC_GPIO_0_REGS, AD7606_BUSY_AD1);

#else
	/* ��ȡI/O״̬ */
	if(GPIOPinIntStatus(SOC_GPIO_0_REGS, AD7606_BUSY_AD2) == GPIO_INT_PEND)
	{
		//��ȡ���ݣ���¼�������Ƿ�ﵽ32�����ݣ��ﵽʱ�����ź���
		//advalue = ((Int16 *)SOC_EMIFA_CS4_ADDR)[0];			//2015-10-14 add by zlb (���ݲɼ���λ����) 
		for(i=0;i<8;i++)
		{
			// get original data 
			advalue = ((Int16 *)SOC_EMIFA_CS4_ADDR)[i];
			// conversion data 
//			ad->databuff[i].channel[ad->conter] = advalue* ad->analog_ad ;
			ad->databuff[i].channel[ad->conter] = advalue;
			ad->databuff[i].channel[ad->conter+1] = 0.0;
		}
		ad->conter += 2;
		if(ad->conter > 63 & ((ad->flagfreq & 0x01) == 0))
		{
			ad->flagfreq |= 0x01;
			Semaphore_post(ad->sem);
			//������
//			GPIOPinWrite(SOC_GPIO_0_REGS, AD7606_CLK_AD1, GPIO_PIN_HIGH);
		}
		else if(ad->conter > 127)
		{
			ad->flagfreq &= ~0x01;
			ad->conter = 0;
			Semaphore_post(ad->sem);
			//������
//			GPIOPinWrite(SOC_GPIO_0_REGS, AD7606_CLK_AD1, GPIO_PIN_HIGH);
		}
		// Ƶ�ʲɼ� 
		if(ad->flagfreq & 0x02)
		{
			ad->freq++;
		}
	}

	/* Set low convst gpio to prepare the next conversion */
	if(ad->flagfreq & (0x01 << 2))
	{
		GPIOPinWrite(SOC_GPIO_0_REGS, AD7606_CLK_AD2, GPIO_PIN_LOW);
	}
	/* Clears the Interrupt Status of AD7606_BUSY_AD1 in GPIO.*/
	GPIOPinIntClear(SOC_GPIO_0_REGS, AD7606_BUSY_AD2);
#endif
}
/***************************************************************************/
//����:void AD7606_Init(float *buff, AD7606Str *ad)
//˵��:��ʼ��AD7606 
//����:buff AD���ݴ���ڴ� ad AD7606�ṹ��ָ��
//���:��
//�༭:zlb
//ʱ��:2015.8.6
/***************************************************************************/
void AD7606_Init(channalstr *buff, AD7606Str *ad)
{
	ad->databuff = buff;
	ad->irq_event = C674X_MASK_INT4 ;
	ad->event_id = AD7606_AD1_BUSY_GPIO_BANK_INT;
	/* ��С�̶�ֵΪ��Χ/16λAD *1000 ��V/Aת��ΪmV/mA */
//	ad->analog_ad = MAX_RANGE*1000.0/65536;
	ad->conter = 0;
	ad->flagfreq = 0; 
	/* �����ź��� */
	ad->sem = Semaphore_create(0,NULL,NULL);
	if(!ad->sem)
	{
		LOG_INFO("failed to create semaphore");
	}
	EMIFA_AD7606_Init();
	AD7606_GPIO_Config();
	AD7606_GPIO_Setup();
	/* IRQ�жϳ�ʼ�� */
	AD7606_IRQ_Setup(ad);
}
