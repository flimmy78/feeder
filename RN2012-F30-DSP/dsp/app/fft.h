#ifndef _FFT_H_
#define  _FFT_H_
#if defined (__cplusplus)
extern "C" {
#endif
#include "sys.h"
#include "queue.h"
#include "fa.h"
/****************************** macro define *****************************/
#define PI                3.14159			//π值
#define PI_180			  0.0174533			//π/180 值
#define F_TOL             (1e-06)			//浮点数极小值
#define TN				   64				//采样64个点 dsplib最大支持128K个点的FFT
#define sqrt2			  1.414				//2开根号

/******************************* type define ******************************/
/* FFT频域值 */
typedef struct _BaseFFT_
{
	float real;								//实部
	float image;							//虚部
}BaseFFTout;
/* 计算出的摸值与相角 */
typedef struct _BaseValue_
{
	float  module;							//模值
	float  angle;							//相角
}BaseValueType;
/* YC结构体 */
struct _YC_STRUCT_
{
	BaseFFTout *fftout;						//fft 频域值
	BaseValueType *modeangel;				//模值与相角
	float *adjust;							//校准指针
	FAPARAMSTR *faparm;						//fa保护参数
};
typedef struct _YC_STRUCT_  YCValueStr;	
/* 队列消息结构体 */
struct _MsgStr_
{
	Queue_Elem elem; 						/* first field for Queue */
	YCValueStr *ycdata; 					/* 遥测数据结构体指针 */
};
typedef struct _MsgStr_ QueueMessStru;

/******************************* declar data ***********************************/

extern float AdjustValue[8];				//校准值数组，一个通道一个校准值
extern Queue *faqueue;						//fft与fa通信队列指针		
extern Semaphore_Handle fasem;				//FFT与FA任务通信信号量
extern YCValueStr ycvalueprt;				//遥测结构体
/******************************* functions **************************************/
Void FFT_Task(UArg arg0, UArg arg1);
Void GetTempture(CurrentPaStr *remotevalue);

//Void FFT(ANSMP_Stru *Input, BaseValueType *Out, UChar flagtmp, float *adjust);


#if defined (__cplusplus)
}
#endif /* _FFT_H_ */
#endif /* _FFT_H_ */
