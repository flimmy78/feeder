/*************************************************************************/
/*
 * Copyright (c) 2015, ֣�����ܵ��� Runner All rights reserved.
 *
 * file       : led.c
 * Design     : zlb
 * Email	  : longbiao831@qq.com
 * Data       : 2015-7-31
 * Version    : V1.0
 * Change     :
 * Description: AD7606���ݲɼ����ֳ���ʵ��
 */
/*************************************************************************/
#include <stdio.h>
#include <string.h>
/* xdctools header files */
#include "hw_types.h"
#include "soc_OMAPL138.h"
#include "gpio.h"
#include "psc.h"
#include "led.h"


/***************************************************************************/
//����:void LED_GPIO_Config(void)
//˵��:��ʼ��LED GPIO����	
//����: ��
//���: ��
//�༭:zlb
//ʱ��:2015.8.1
/***************************************************************************/
void LED_GPIO_Config(void)
{
	volatile unsigned int savePinMux = 0; 
	
	/* enable gpio psc */
	PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_GPIO, PSC_POWERDOMAIN_ALWAYS_ON,
			PSC_MDCTL_NEXT_ENABLE);
	
	/*selects the LED pins for use*/
	savePinMux = HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(5)) &
		~(SYSCFG_PINMUX5_PINMUX5_23_20 | SYSCFG_PINMUX5_PINMUX5_15_12);

	/* config SCLK and DATA for ad1 */
	HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(5)) =
		(PINMUX5_LED_SCLK_ENABLE | PINMUX5_LED_DATA_ENABLE | savePinMux);

	/*Set the AD7606 I/O direction */
	GPIODirModeSet(SOC_GPIO_0_REGS, LED_SCLK, GPIO_DIR_OUTPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, LED_DATA, GPIO_DIR_OUTPUT);
}
/***************************************************************************/
//����:void LED_SENDOUT(UChar led_data)
//˵��:���LED�ź�
//����:LED״̬
//���: ��
//�༭:zlb
//ʱ��:2015.8.1
/***************************************************************************/
void LED_SENDOUT(UChar led_data)
{
	UChar i;

	for(i=0; i<8; i++)
	{
		GPIOPinWrite(SOC_GPIO_0_REGS, LED_SCLK, GPIO_PIN_LOW);
		/* ÿ�η���һλ���� */
		GPIOPinWrite(SOC_GPIO_0_REGS, LED_DATA, (led_data >> (7-i))&0x01 );
		/* ��ʱ18ns���� */
		Delay(10);
		GPIOPinWrite(SOC_GPIO_0_REGS, LED_SCLK, GPIO_PIN_HIGH);
		Delay(10);
	}
}
/***************************************************************************/
//����:void Delay(unsigned int n)
//˵��:��ʱn��ʱ�ӽ��� 456MƵ���� һ������2.2ns	
//����: ��ʱʱ�������
//���: ��
//�༭:zlb
//ʱ��:2015.8.1
/***************************************************************************/
void Delay(volatile unsigned int n)
{
    while(n--);
}
/***************************************************************************/
//����:void YX_GPIO_Config(void)
//˵��:��ʼ��YX GPIO����	
//����: ��
//���: ��
//�༭:zlb
//ʱ��:2015.8.1
/***************************************************************************/
void YX_GPIO_Config(void)
{
	volatile unsigned int savePinMux = 0; 
	
	/* enable gpio psc */
	PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_GPIO, PSC_POWERDOMAIN_ALWAYS_ON,
			PSC_MDCTL_NEXT_ENABLE);
	
	/* selects the YX1 pins for use */
	savePinMux = HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1)) &
		~(SYSCFG_PINMUX1_PINMUX1_23_20 | SYSCFG_PINMUX1_PINMUX1_19_16 | \
		  SYSCFG_PINMUX1_PINMUX1_15_12 | SYSCFG_PINMUX1_PINMUX1_31_28 | \
		  SYSCFG_PINMUX1_PINMUX1_3_0);
	HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1)) =
		(PINMUX1_YX1_ENABLE | PINMUX1_YX2_ENABLE | \
		 PINMUX1_YX3_ENABLE	| PINMUX1_YX4_ENABLE | \
		 savePinMux);
	
	/* selects the YX2-YX9 for use */
	savePinMux = HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(0)) & \
		~(SYSCFG_PINMUX0_PINMUX0_15_12 | SYSCFG_PINMUX0_PINMUX0_11_8  | \
		  SYSCFG_PINMUX0_PINMUX0_7_4   | SYSCFG_PINMUX0_PINMUX0_3_0);
	HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1)) =
		(PINMUX0_YX7_ENABLE | PINMUX0_YX8_ENABLE | \
		 PINMUX0_YX9_ENABLE | PINMUX0_YX10_ENABLE | \
		 savePinMux);

	/* selects the YX5 pins for use */
	savePinMux = HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(2)) &
		~(SYSCFG_PINMUX2_PINMUX2_3_0);
	HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(2)) =
		(PINMUX2_YX5_ENABLE | savePinMux);


	/*Set the YX I/O direction */
	GPIODirModeSet(SOC_GPIO_0_REGS, YX_1, GPIO_DIR_INPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, YX_2, GPIO_DIR_INPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, YX_3, GPIO_DIR_INPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, YX_4, GPIO_DIR_INPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, YX_5, GPIO_DIR_INPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, YX_6, GPIO_DIR_INPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, YX_7, GPIO_DIR_INPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, YX_8, GPIO_DIR_INPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, YX_9, GPIO_DIR_INPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, YX_10, GPIO_DIR_INPUT);
}
/***************************************************************************/
//����:static void YK_GPIO_Config(void)
//˵��:��ʼ��YK GPIO����	
//����: ��
//���: ��
//�༭:zlb
//ʱ��:2015.8.1
/***************************************************************************/
void YK_GPIO_Config(void)
{
	volatile unsigned int savePinMux = 0; 
	
	/* enable gpio psc */
	PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_GPIO, PSC_POWERDOMAIN_ALWAYS_ON,
			PSC_MDCTL_NEXT_ENABLE);
	
	/* selects the YKZS pins for use */
	savePinMux = HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(19)) &
		~(SYSCFG_PINMUX19_PINMUX19_15_12 | SYSCFG_PINMUX19_PINMUX19_11_8 | \
		  SYSCFG_PINMUX19_PINMUX19_19_16 );
	HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(19)) =
		(PINMUX19_YZS_ENABLE | PINMUX19_FZ1_ENABLE | \
		 PINMUX19_HZ1_ENABLE | savePinMux);

	/* selects the YZ - FZ2 pins for use */
	savePinMux = HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1)) &
		~(SYSCFG_PINMUX1_PINMUX1_27_24);
	HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(1)) =
		(PINMUX1_YZ_ENABLE | savePinMux);

	/* selects the HZ2 FZ2 pins for use */
	savePinMux = HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(14)) &
		~(SYSCFG_PINMUX14_PINMUX14_3_0 | SYSCFG_PINMUX14_PINMUX14_7_4 );
	HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(14)) =
		(PINMUX14_HZ2_ENABLE | PINMUX14_FZ2_ENABLE | \
		 savePinMux);

	/*Set the Yk I/O direction */
	GPIODirModeSet(SOC_GPIO_0_REGS, YK_YZ, GPIO_DIR_OUTPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, YK_YZS, GPIO_DIR_INPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, YK_FZ1, GPIO_DIR_OUTPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, YK_HZ1, GPIO_DIR_OUTPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, YK_HZ2, GPIO_DIR_OUTPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, YK_FZ2, GPIO_DIR_OUTPUT);
	/* Set high level */
	GPIOPinWrite(SOC_GPIO_0_REGS, YK_YZ, GPIO_PIN_HIGH);
	GPIOPinWrite(SOC_GPIO_0_REGS, YK_FZ1, GPIO_PIN_HIGH);
	GPIOPinWrite(SOC_GPIO_0_REGS, YK_HZ1, GPIO_PIN_HIGH);
	GPIOPinWrite(SOC_GPIO_0_REGS, YK_HZ2, GPIO_PIN_HIGH);
	GPIOPinWrite(SOC_GPIO_0_REGS, YK_FZ2, GPIO_PIN_HIGH);
}
/***************************************************************************/
//����:void YK_SendOut(UInt16 index, UInt16 value)
//˵��:ң�����
//����: index ң�ر�� value ����ߵ͵�ƽ
//���: ��
//�༭:zlb
//ʱ��:2015.8.15
/***************************************************************************/
void YK_SendOut(UInt16 index, UInt16 value)
{
	UInt16 pin;
	if(index > 4)
		return;
	switch(index)
	{
		case 0:					//Ԥ��
			pin = YK_YZ;
			break;
		case 1:					// 1 ��
			pin = YK_HZ1;
			break;
		case 2:					// 1 ��
			pin = YK_FZ1;		
			break;
		case 3:					// 2 ��
			pin = YK_HZ2;
			break;
		case 4:					// 2 ��
			pin = YK_FZ2;
			break;
	}

	if(value == PIN_HIGH)
		GPIOPinWrite(SOC_GPIO_0_REGS, pin, GPIO_PIN_HIGH);
	else
		GPIOPinWrite(SOC_GPIO_0_REGS, pin, GPIO_PIN_LOW);
}


