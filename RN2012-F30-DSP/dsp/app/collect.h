/*************************************************************************/
/*
 * Copyright (c) 2015, ֣�����ܵ��� Runner All rights reserved.
 *
 * file       : collect.h
 * Design     : zlb
 * Email	  : longbiao831@qq.com
 * Data       : 2015-8-10
 * Version    : V1.0
 * Change     :
 * Description: ң�⡢ң�����ݲɼ� Collect_Task�������г���ͷ�ļ�
 */
/*************************************************************************/
#ifndef _COLLECT_H_
#define _COLLECT_H_
#include <ti/sysbios/BIOS.h>

#define INDEX_FZ1		1				//��բ1ң��ͨ����
#define INDEX_HZ1		2				//��բ1ң��ͨ����
#define INDEX_FZ2		3				//��բ2ң��ͨ����
#define INDEX_HZ2		4				//��բ2ң��ͨ����

//�����źű�־
extern UChar FGflag;




Void Collect_Task(UArg arg0, UArg arg1);
Void ChangeLED(UInt32 *status);

#endif /* _COLLECT_H_ */

