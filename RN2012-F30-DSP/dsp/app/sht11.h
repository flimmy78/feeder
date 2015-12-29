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
#ifndef _SHT11_H_
#define _SHT11_H_

#include <xdc/std.h>
#include "hw_types.h"                // 宏命令
#include "hw_syscfg0_C6748.h"        // 系统配置模块寄存器
#include "soc_C6748.h"               // DSP C6748 外设寄存器
#include "gpio.h"


#define TEMP_CMD  0x03			//温度测量 000 地址 00011 cmd
#define HUMI_CMD  0x05			//湿度测量 000 地址 00101 cmd
#define RREG_CMD  0x07			//读状态寄存器 000 地址 00111 cmd
#define WREG_CMD  0x06			//写状态寄存器 000 地址 00110 cmd
#define RESET_CMD 0x1E			//软复位， 000 地址 11110 cmd 下次发送命令至少等待11ms
#define REG_Resolution  0x01	//状态寄存器分辨率位 1 (8bit 湿度 12bit 温度) 0 (12bit 湿度 14bit 温度 )
/* 测量不同精度下需要的等待测量时间 */
#define TEMP_DELAY_8	20		//8bit
#define TEMP_DELAY_12	80		//12bit
#define TEMP_DELAY_14	320u	//14bit


#define SCK			GPIO_TO_PIN(1, 5)
#define DATA		GPIO_TO_PIN(1, 4)
/* IO管脚方向设置 */
#define SetDirection_OUT(PINMUX4_IO)  	GPIODirModeSet(SOC_GPIO_0_REGS, PINMUX4_IO, GPIO_DIR_OUTPUT)
#define SetDirection_IN(PINMUX4_IO)		GPIODirModeSet(SOC_GPIO_0_REGS, PINMUX4_IO, GPIO_DIR_INPUT)
/* IO管脚输出高低电平 */
#define SetValue_HIGH(PIN)					GPIOPinWrite(SOC_GPIO_0_REGS, PIN, GPIO_PIN_HIGH)
#define SetValue_LOW(PIN)					GPIOPinWrite(SOC_GPIO_0_REGS, PIN, GPIO_PIN_LOW)
/* 读取IO管脚状态 */
#define ReadValue(PIN)						GPIOPinRead(SOC_GPIO_0_REGS, PIN)



/* 函数声明 */
void TEMP_GPIO_Config(void);
UInt16 Measure_Data(UChar cmd, UInt16 ms);
float Convert_Temp(UInt16 data);
float Convert_Humi(UInt16 data, float temp);
float Convert_Temp12(UInt16 data);
float Convert_Humi8(UInt16 data, float temp);

#endif/* _SHT11_H_ */

