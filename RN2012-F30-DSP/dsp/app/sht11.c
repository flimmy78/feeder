/*************************************************************************/
/*
 * Copyright (c) 2015, 郑州瑞能电气 Runner All rights reserved.
 *
 * file       : sht11.h
 * Design     : zlb
 * Email	  : longbiao831@qq.com
 * Data       : 2015-8-3
 * Version    : V1.0
 * Change     :
 * Description: SHT11温湿度采集部分头文件
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
//函数:void TEMP_GPIO_Config(void)
//说明:初始化SHT10 温湿度传感器 GPIO设置	
//输入: 无
//输出: 无
//编辑:zlb
//时间:2015.8.1
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
//函数:static void Start_Sensor(void)
//说明:启动温湿度传感器
//输入: 无
//输出: 无
//编辑:zlb
//时间:2015.8.3
/***************************************************************************/
static void Start_Sensor(void)
{
	SetDirection_OUT(DATA);
		
	SetValue_HIGH(SCK);
	SetValue_HIGH(DATA);
	/* 延时时间大于100ns/2 */
	Delay(50);
	SetValue_LOW(DATA);
	Delay(50);
	SetValue_LOW(SCK);	
	/* 延时时间大于100ns */
	Delay(100);
	SetValue_HIGH(SCK);
	Delay(50);
	SetValue_HIGH(DATA);	
}
/***************************************************************************/
//函数:static void Reset_Soft(void)
//说明:通讯复位
//输入: 无
//输出: 无
//编辑:zlb
//时间:2015.8.3
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
//函数:static void Send_CMD(UChar cmd)
//说明:发送cmd命令到传感器
//输入: cmd 输入命令
//输出: 无
//编辑:zlb
//时间:2015.8.3
/***************************************************************************/
static void Send_CMD(UChar cmd)
{
	UChar i;

	SetDirection_OUT(DATA);
	SetValue_LOW(SCK);
	//是否需要延时?
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
//函数:static void SHT11_Answer(void)
//说明:应答信号 相当于iic的ack信号
//输入: 无
//输出: 无
//编辑:zlb
//时间:2015.8.3
/***************************************************************************/
static void SHT11_Answer(void)
{
	SetDirection_IN(DATA);
	
	SetValue_HIGH(SCK);
	Delay(50);
	/* 等待DATA管脚为低电平 证明收到ACK信号 */
	while(ReadValue(DATA) == 1);
	SetValue_LOW(SCK);
	SetValue_HIGH(DATA);
	/* 改变IO管脚为输入 */
	
}
/***************************************************************************/
//函数:UChar Receive_Data(void)
//说明:接收8bit数据
//输入: 无
//输出:接收到的8bit数据
//编辑:zlb
//时间:2015.8.3
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
//函数:static void MCU_Answer(void)
//说明:接收8bit数据后 MCU的应答
//输入: 无
//输出: 无
//编辑:zlb
//时间:2015.8.3
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
//函数:static void End_Receive(void)
//说明:停止接收数据
//输入: 无
//输出: 无
//编辑:zlb
//时间:2015.8.3
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
//函数:static void Write_Register(UChar data)
//说明:写传感器寄存器 data 为写寄存器值 bit2 加热 bit1 OTP bit0 分辨率
//输入:data 写入状态寄存器值
//输出: 无
//编辑:zlb
//时间:2015.8.3
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
//函数:static :UChar Read_Register(void)
//说明:读传感器寄存器 
//输入:无
//输出:寄存器值
//编辑:zlb
//时间:2015.8.3
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
//函数:UInt16 Measure_Data(UChar cmd, UInt16 ms)
//说明:测量温湿度数据，8/12/14bit 数据都可以
//输入: cmd 命令(采集温度或湿度) ms 延时时间
//输出: 测量值
//编辑:zlb
//时间:2015.8.3
/***************************************************************************/
UInt16 Measure_Data(UChar cmd, UInt16 ms)
{
	UInt16 data = 0;

	Start_Sensor();
	Send_CMD(cmd);
	SHT11_Answer();
	/* 等待数据准备完毕 */
	Task_sleep(ms);
	/* 等待data线被拉低 */
	while(ReadValue(DATA) == 1);	
	data = Receive_Data();
	data <<= 8;
	MCU_Answer();
	data += Receive_Data();
	End_Receive();

	return data;
}
/***************************************************************************/
//函数:float Convert_Temp(UInt16 data)
//说明:测量数据转换为温度值 本转换是14bit精度的采样 公式:T=d1+d2*SOt d1=-39.6
//     d2=0.01	SOt=data;	
//输入: data 采集到的原始数据
//输出: 温度值
//编辑:zlb
//时间:2015.8.3
/***************************************************************************/
float Convert_Temp(UInt16 data)
{
	float temperature;

	temperature = -39.6+0.01*data;

	return temperature;
}
/***************************************************************************/
//函数:float Convert_Humi(UInt16 data, float temp)
//说明:测量数据转换为湿度值 转换精度为12bit精度采样 公式:RHline=C1+C2*SOrh+C3*SOrh*SOrh
//     RHture = (T-25)*(t1+t2*SOrh)+RHline  C1=-2.0468 C2=0.0367 C3=-1.5955/1000000
//     t1=0.01 t2=0.00008
//输入: 无
//输出: 无
//编辑:zlb
//时间:2015.8.3
/***************************************************************************/
float Convert_Humi(UInt16 data, float temp)
{
	float RHlinear,RHture;

	RHlinear = -2.0468+0.0367*data-0.000001596*data*data;
	RHture = (temp - 25)*(0.01+0.00008*data)+RHlinear;

	return RHture;
}
/***************************************************************************/
//函数:float Convert_Temp(UInt16 data)
//说明:测量数据转换为温度值 本转换是12bit精度的采样 公式:T=d1+d2*SOt d1=-39.6
//     d2=0.04	SOt=data;	
//输入: data 采集到的原始数据
//输出: 温度值
//编辑:zlb
//时间:2015.8.3
/***************************************************************************/
float Convert_Temp12(UInt16 data)
{
	float temperature;

	temperature = -39.6+0.04*data;

	return temperature;
}
/***************************************************************************/
//函数:float Convert_Humi(UInt16 data, float temp)
//说明:测量数据转换为湿度值 转换精度为8bit精度采样 公式:RHline=C1+C2*SOrh+C3*SOrh*SOrh
//     RHture = (T-25)*(t1+t2*SOrh)+RHline  C1=-2.0468 C2=0.5872 C3=-4.0845/10000
//     t1=0.01 t2=0.00128
//输入: 无
//输出: 无
//编辑:zlb
//时间:2015.8.3
/***************************************************************************/
float Convert_Humi8(UInt16 data, float temp)
{
	float RHlinear,RHture;

	RHlinear = -2.0468+0.5872*data-0.00040845*data*data;
	RHture = (temp - 25)*(0.01+0.00128*data)+RHlinear;

	return RHture;
}
