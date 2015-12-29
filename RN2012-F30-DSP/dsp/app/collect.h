/*************************************************************************/
/*
 * Copyright (c) 2015, 郑州瑞能电气 Runner All rights reserved.
 *
 * file       : collect.h
 * Design     : zlb
 * Email	  : longbiao831@qq.com
 * Data       : 2015-8-10
 * Version    : V1.0
 * Change     :
 * Description: 遥测、遥信数据采集 Collect_Task任务运行程序头文件
 */
/*************************************************************************/
#ifndef _COLLECT_H_
#define _COLLECT_H_
#include <ti/sysbios/BIOS.h>

#define INDEX_FZ1		1				//分闸1遥信通道号
#define INDEX_HZ1		2				//合闸1遥信通道号
#define INDEX_FZ2		3				//分闸2遥信通道号
#define INDEX_HZ2		4				//合闸2遥信通道号

//复归信号标志
extern UChar FGflag;




Void Collect_Task(UArg arg0, UArg arg1);
Void ChangeLED(UInt32 *status);

#endif /* _COLLECT_H_ */

