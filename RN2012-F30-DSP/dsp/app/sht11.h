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
#ifndef _SHT11_H_
#define _SHT11_H_

#include <xdc/std.h>
#include "hw_types.h"                // ������
#include "hw_syscfg0_C6748.h"        // ϵͳ����ģ��Ĵ���
#include "soc_C6748.h"               // DSP C6748 ����Ĵ���
#include "gpio.h"


#define TEMP_CMD  0x03			//�¶Ȳ��� 000 ��ַ 00011 cmd
#define HUMI_CMD  0x05			//ʪ�Ȳ��� 000 ��ַ 00101 cmd
#define RREG_CMD  0x07			//��״̬�Ĵ��� 000 ��ַ 00111 cmd
#define WREG_CMD  0x06			//д״̬�Ĵ��� 000 ��ַ 00110 cmd
#define RESET_CMD 0x1E			//��λ�� 000 ��ַ 11110 cmd �´η����������ٵȴ�11ms
#define REG_Resolution  0x01	//״̬�Ĵ����ֱ���λ 1 (8bit ʪ�� 12bit �¶�) 0 (12bit ʪ�� 14bit �¶� )
/* ������ͬ��������Ҫ�ĵȴ�����ʱ�� */
#define TEMP_DELAY_8	20		//8bit
#define TEMP_DELAY_12	80		//12bit
#define TEMP_DELAY_14	320u	//14bit


#define SCK			GPIO_TO_PIN(1, 5)
#define DATA		GPIO_TO_PIN(1, 4)
/* IO�ܽŷ������� */
#define SetDirection_OUT(PINMUX4_IO)  	GPIODirModeSet(SOC_GPIO_0_REGS, PINMUX4_IO, GPIO_DIR_OUTPUT)
#define SetDirection_IN(PINMUX4_IO)		GPIODirModeSet(SOC_GPIO_0_REGS, PINMUX4_IO, GPIO_DIR_INPUT)
/* IO�ܽ�����ߵ͵�ƽ */
#define SetValue_HIGH(PIN)					GPIOPinWrite(SOC_GPIO_0_REGS, PIN, GPIO_PIN_HIGH)
#define SetValue_LOW(PIN)					GPIOPinWrite(SOC_GPIO_0_REGS, PIN, GPIO_PIN_LOW)
/* ��ȡIO�ܽ�״̬ */
#define ReadValue(PIN)						GPIOPinRead(SOC_GPIO_0_REGS, PIN)



/* �������� */
void TEMP_GPIO_Config(void);
UInt16 Measure_Data(UChar cmd, UInt16 ms);
float Convert_Temp(UInt16 data);
float Convert_Humi(UInt16 data, float temp);
float Convert_Temp12(UInt16 data);
float Convert_Humi8(UInt16 data, float temp);

#endif/* _SHT11_H_ */

