/*************************************************************************/
/*
 * Copyright (c) 2015, ֣�����ܵ��� Runner All rights reserved.
 *
 * file       : sht11.h
 * Design     : zlb
 * Email	  : longbiao831@qq.com
 * Data       : 2015-8-3
 * Version    : V1.0
 * Change     :
 * Description: SHT11��ʪ�Ȳɼ�����ͷ�ļ�
 */
/*************************************************************************/
/* sysbios header files */
#include <ti/sysbios/BIOS.h>
#include <ti/sysbios/knl/Task.h>
#include "hw_types.h"
#include "soc_OMAPL138.h"
#include "EMIFAPinmuxSetup.h"
#include "gpio.h"
#include "psc.h"
#include "string.h"
#include "sht11.h"
#include "led.h"


/***************************************************************************/
//����:void TEMP_GPIO_Config(void)
//˵��:��ʼ��SHT10 ��ʪ�ȴ����� GPIO����	
//����: ��
//���: ��
//�༭:zlb
//ʱ��:2015.8.1
/***************************************************************************/
void TEMP_GPIO_Config(void)
{
	volatile unsigned int savePinMux = 0; 
	
	/* enable gpio psc */
	PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_GPIO, PSC_POWERDOMAIN_ALWAYS_ON,
			PSC_MDCTL_NEXT_ENABLE);
	
	/*selects the LED pins for use*/
	savePinMux = HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(4)) &
		~(SYSCFG_PINMUX4_PINMUX4_11_8 | SYSCFG_PINMUX4_PINMUX4_15_12);

	/* config SCLK and DATA for ad1 */
	HWREG(SOC_SYSCFG_0_REGS + SYSCFG0_PINMUX(4)) =
		(PINMUX4_TEMP_SCLK_ENABLE | PINMUX4_TEMP_DATA_ENABLE | savePinMux);

	/*Set the SHT10 I/O direction */
	GPIODirModeSet(SOC_GPIO_0_REGS, SCK, GPIO_DIR_OUTPUT);
	GPIODirModeSet(SOC_GPIO_0_REGS, DATA, GPIO_DIR_INPUT);
}
/***************************************************************************/
//����:static void Start_Sensor(void)
//˵��:������ʪ�ȴ�����
//����: ��
//���: ��
//�༭:zlb
//ʱ��:2015.8.3
/***************************************************************************/
static void Start_Sensor(void)
{
	SetDirection_OUT(DATA);
		
	SetValue_HIGH(SCK);
	SetValue_HIGH(DATA);
	/* ��ʱʱ�����100ns/2 */
	Delay(50);
	SetValue_LOW(DATA);
	Delay(50);
	SetValue_LOW(SCK);	
	/* ��ʱʱ�����100ns */
	Delay(100);
	SetValue_HIGH(SCK);
	Delay(50);
	SetValue_HIGH(DATA);	
}
/***************************************************************************/
//����:static void Reset_Soft(void)
//˵��:ͨѶ��λ
//����: ��
//���: ��
//�༭:zlb
//ʱ��:2015.8.3
/***************************************************************************/
static void Reset_Soft(void)
{
	UChar i;

	SetDirection_OUT(DATA);
	SetValue_HIGH(DATA);
	SetValue_LOW(SCK);
	for(i=0;i<10;i++)
	{
		SetValue_HIGH(SCK);
		Delay(100);
		SetValue_LOW(SCK);
		Delay(100);
	}
	Start_Sensor();
}
/***************************************************************************/
//����:static void Send_CMD(UChar cmd)
//˵��:����cmd���������
//����: cmd ��������
//���: ��
//�༭:zlb
//ʱ��:2015.8.3
/***************************************************************************/
static void Send_CMD(UChar cmd)
{
	UChar i;

	SetDirection_OUT(DATA);
	SetValue_LOW(SCK);
	//�Ƿ���Ҫ��ʱ?
	for(i=0;i<8;i++)
	{
		if(cmd & 0x80)
		{
			SetValue_HIGH(DATA);
			/* 150ns */
			Delay(100);
		}
		else
		{
			SetValue_LOW(DATA);
			Delay(100);
		}
		cmd <<= 1;
		SetValue_HIGH(SCK);
		Delay(100);
		SetValue_LOW(SCK);
		/* 15ns */
		Delay(10);
	}
}
/***************************************************************************/
//����:static void SHT11_Answer(void)
//˵��:Ӧ���ź� �൱��iic��ack�ź�
//����: ��
//���: ��
//�༭:zlb
//ʱ��:2015.8.3
/***************************************************************************/
static void SHT11_Answer(void)
{
	SetDirection_IN(DATA);
	
	SetValue_HIGH(SCK);
	Delay(50);
	/* �ȴ�DATA�ܽ�Ϊ�͵�ƽ ֤���յ�ACK�ź� */
	while(ReadValue(DATA) == 1);
	SetValue_LOW(SCK);
	SetValue_HIGH(DATA);
	/* �ı�IO�ܽ�Ϊ���� */
	
}
/***************************************************************************/
//����:UChar Receive_Data(void)
//˵��:����8bit����
//����: ��
//���:���յ���8bit����
//�༭:zlb
//ʱ��:2015.8.3
/***************************************************************************/
UChar Receive_Data(void)
{
	UChar i;
	UChar data = 0;

	SetValue_LOW(SCK);
	Delay(1);
	for(i=0;i<8;i++)
	{
		SetValue_HIGH(SCK);
		Delay(100);
		data <<= 1;
		if(ReadValue(DATA) == 1)
		{
			data |= 0x01;
		}
		else
		{
			data &= 0xfe;
		}
		SetValue_LOW(SCK);
		Delay(100);
	}
	return data;
}
/***************************************************************************/
//����:static void MCU_Answer(void)
//˵��:����8bit���ݺ� MCU��Ӧ��
//����: ��
//���: ��
//�༭:zlb
//ʱ��:2015.8.3
/***************************************************************************/
static void MCU_Answer(void)
{
	SetDirection_OUT(DATA);

	SetValue_HIGH(DATA);
	SetValue_LOW(SCK);
	Delay(50);
	SetValue_LOW(DATA);
	Delay(1);
	SetValue_HIGH(SCK);
	Delay(100);
	SetValue_LOW(SCK);
	Delay(1);
	SetValue_HIGH(DATA);

	SetDirection_IN(DATA);
}
/***************************************************************************/
//����:static void End_Receive(void)
//˵��:ֹͣ��������
//����: ��
//���: ��
//�༭:zlb
//ʱ��:2015.8.3
/***************************************************************************/
static void End_Receive(void)
{
	SetDirection_OUT(DATA);

	SetValue_HIGH(DATA);
	SetValue_HIGH(SCK);
	Delay(100);
	SetValue_LOW(SCK);
}
/***************************************************************************/
//����:static void Write_Register(UChar data)
//˵��:д�������Ĵ��� data Ϊд�Ĵ���ֵ bit2 ���� bit1 OTP bit0 �ֱ���
//����:data д��״̬�Ĵ���ֵ
//���: ��
//�༭:zlb
//ʱ��:2015.8.3
/***************************************************************************/
static void Write_Register(UChar data)
{
	Start_Sensor();
	Send_CMD(WREG_CMD);
	SHT11_Answer();
	Send_CMD(data);
	SHT11_Answer();
}
/***************************************************************************/
//����:static :UChar Read_Register(void)
//˵��:���������Ĵ��� 
//����:��
//���:�Ĵ���ֵ
//�༭:zlb
//ʱ��:2015.8.3
/***************************************************************************/
static UChar Read_Register(void)
{
	UChar data = 0;
	
	Start_Sensor();
	Send_CMD(RREG_CMD);
	SHT11_Answer();
	data = Receive_Data();
	End_Receive();
	
	return data;
}
/***************************************************************************/
//����:UInt16 Measure_Data(UChar cmd, UInt16 ms)
//˵��:������ʪ�����ݣ�8/12/14bit ���ݶ�����
//����: cmd ����(�ɼ��¶Ȼ�ʪ��) ms ��ʱʱ��
//���: ����ֵ
//�༭:zlb
//ʱ��:2015.8.3
/***************************************************************************/
UInt16 Measure_Data(UChar cmd, UInt16 ms)
{
	UInt16 data = 0;

	Start_Sensor();
	Send_CMD(cmd);
	SHT11_Answer();
	/* �ȴ�����׼����� */
	Task_sleep(ms);
	/* �ȴ�data�߱����� */
	while(ReadValue(DATA) == 1);	
	data = Receive_Data();
	data <<= 8;
	MCU_Answer();
	data += Receive_Data();
	End_Receive();

	return data;
}
/***************************************************************************/
//����:float Convert_Temp(UInt16 data)
//˵��:��������ת��Ϊ�¶�ֵ ��ת����14bit���ȵĲ��� ��ʽ:T=d1+d2*SOt d1=-39.6
//     d2=0.01	SOt=data;	
//����: data �ɼ�����ԭʼ����
//���: �¶�ֵ
//�༭:zlb
//ʱ��:2015.8.3
/***************************************************************************/
float Convert_Temp(UInt16 data)
{
	float temperature;

	temperature = -39.6+0.01*data;

	return temperature;
}
/***************************************************************************/
//����:float Convert_Humi(UInt16 data, float temp)
//˵��:��������ת��Ϊʪ��ֵ ת������Ϊ12bit���Ȳ��� ��ʽ:RHline=C1+C2*SOrh+C3*SOrh*SOrh
//     RHture = (T-25)*(t1+t2*SOrh)+RHline  C1=-2.0468 C2=0.0367 C3=-1.5955/1000000
//     t1=0.01 t2=0.00008
//����: ��
//���: ��
//�༭:zlb
//ʱ��:2015.8.3
/***************************************************************************/
float Convert_Humi(UInt16 data, float temp)
{
	float RHlinear,RHture;

	RHlinear = -2.0468+0.0367*data-0.000001596*data*data;
	RHture = (temp - 25)*(0.01+0.00008*data)+RHlinear;

	return RHture;
}
/***************************************************************************/
//����:float Convert_Temp(UInt16 data)
//˵��:��������ת��Ϊ�¶�ֵ ��ת����12bit���ȵĲ��� ��ʽ:T=d1+d2*SOt d1=-39.6
//     d2=0.04	SOt=data;	
//����: data �ɼ�����ԭʼ����
//���: �¶�ֵ
//�༭:zlb
//ʱ��:2015.8.3
/***************************************************************************/
float Convert_Temp12(UInt16 data)
{
	float temperature;

	temperature = -39.6+0.04*data;

	return temperature;
}
/***************************************************************************/
//����:float Convert_Humi(UInt16 data, float temp)
//˵��:��������ת��Ϊʪ��ֵ ת������Ϊ8bit���Ȳ��� ��ʽ:RHline=C1+C2*SOrh+C3*SOrh*SOrh
//     RHture = (T-25)*(t1+t2*SOrh)+RHline  C1=-2.0468 C2=0.5872 C3=-4.0845/10000
//     t1=0.01 t2=0.00128
//����: ��
//���: ��
//�༭:zlb
//ʱ��:2015.8.3
/***************************************************************************/
float Convert_Humi8(UInt16 data, float temp)
{
	float RHlinear,RHture;

	RHlinear = -2.0468+0.5872*data-0.00040845*data*data;
	RHture = (temp - 25)*(0.01+0.00128*data)+RHlinear;

	return RHture;
}
