#ifndef _FFT_H_
#define  _FFT_H_
#if defined (__cplusplus)
extern "C" {
#endif
#include "sys.h"
#include "queue.h"
#include "fa.h"
/****************************** macro define *****************************/
#define PI                3.14159			//��ֵ
#define PI_180			  0.0174533			//��/180 ֵ
#define F_TOL             (1e-06)			//��������Сֵ
#define TN				   64				//����64���� dsplib���֧��128K�����FFT
#define sqrt2			  1.414				//2������

/******************************* type define ******************************/
/* FFTƵ��ֵ */
typedef struct _BaseFFT_
{
	float real;								//ʵ��
	float image;							//�鲿
}BaseFFTout;
/* ���������ֵ����� */
typedef struct _BaseValue_
{
	float  module;							//ģֵ
	float  angle;							//���
}BaseValueType;
/* YC�ṹ�� */
struct _YC_STRUCT_
{
	BaseFFTout *fftout;						//fft Ƶ��ֵ
	BaseValueType *modeangel;				//ģֵ�����
	float *adjust;							//У׼ָ��
	FAPARAMSTR *faparm;						//fa��������
};
typedef struct _YC_STRUCT_  YCValueStr;	
/* ������Ϣ�ṹ�� */
struct _MsgStr_
{
	Queue_Elem elem; 						/* first field for Queue */
	YCValueStr *ycdata; 					/* ң�����ݽṹ��ָ�� */
};
typedef struct _MsgStr_ QueueMessStru;

/******************************* declar data ***********************************/

extern float AdjustValue[8];				//У׼ֵ���飬һ��ͨ��һ��У׼ֵ
extern Queue *faqueue;						//fft��faͨ�Ŷ���ָ��		
extern Semaphore_Handle fasem;				//FFT��FA����ͨ���ź���
extern YCValueStr ycvalueprt;				//ң��ṹ��
/******************************* functions **************************************/
Void FFT_Task(UArg arg0, UArg arg1);
Void GetTempture(CurrentPaStr *remotevalue);

//Void FFT(ANSMP_Stru *Input, BaseValueType *Out, UChar flagtmp, float *adjust);


#if defined (__cplusplus)
}
#endif /* _FFT_H_ */
#endif /* _FFT_H_ */
