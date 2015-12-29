#ifndef _EMIF_FPGA_H_
#define _EMIF_FPGA_H_
#if defined (__cplusplus)
extern "C" {
#endif
#include <ti/sysbios/knl/Queue.h>
#include "queue.h"

//reserve cmd out define
//#define DEBUG_RESERVE

//without of FPGA data to test 
#define DEBUG_NOFPGA			  1

/* EMIF CS2 base address */
#define EMIF_BASEADDR  		  0x60000000
/* base message logo, yc ready state, yx ready state, microsecond, yk status, yk output, versions, Modefpga  */
#define OFFSET_LOGO              (0x0)    /* dtu versions "RN1080" */
#define OFFSET_DATAREADY         (0x6)    /* analogy data ready address 1 ready 0 noready */
#define OFFSET_DIGITREADY        (0x7)    /* digit data ready address 1 ready 0 no */
#define OFFSET_MICROSND          (0xc)    /* microsecond data 4Byte */
#define OFFSET_CONTRREADY1       (0x10)   /* remote control status board 1 "00" open "11" close 4Byte */
#define OFFSET_CONTRREADY2       (0x14)   /* remote control status board 2 "00" open "11" close 4Byte */
#define OFFSET_CONTRCMD          (0x18)   /* remote control command 4Byte */
#define OFFSET_VERSION           (0x20)   /* board versions 0200 4Byte */
#define OFFSET_MODEFPGA          (0x2c)   /* fpga run mode 4Byte */
#define OFFSET_CONTRCMDNEW1	   (0x30)	/* remote control cmd out board 1 "00" open "11" close 4Byte */
#define OFFSET_CONTRCMDNEW2	   (0x34)	/* remote control cmd out board 2 "00" open "11" close 4Byte */
/* 32 group yx data */
#define OFFSET_DIGRP_BASE        (0x40)   /* digit group data base address */
#define OFFSET_DIGRP_STEP        (0x10)   /* digit group data step-by-step value */
/* board frequency and set the period of translate period */
#define OFFSET_FREQ1             (0x2e0u) /* frequency of board one 2Byte 3 point*/
#define OFFSET_FREQ2             (0x2e2u) /* frequency of board two 2Byte 3 point*/
#define OFFSET_FREQ3             (0x2e4u) /* frequency of board three 2Byte 3 point*/
#define OFFSET_FREQ4             (0x2e6u) /* frequency of board four 2Byte 3 point*/
#define OFFSET_FREQSET1          (0x2e8u) /* frequency to translate period of board two 2Byte 0.1microsecond*/
#define OFFSET_FREQSET2          (0x2eau) /* frequency to translate period of board two 2Byte 0.1microsecond*/
#define OFFSET_FREQSET3          (0x2ecu) /* frequency to translate period of board two 2Byte 0.1microsecond*/
#define OFFSET_FREQSET4          (0x2eeu) /* frequency to translate period of board two 2Byte 0.1microsecond*/
/* newest num of yx and yc */
#define OFFSET_NEWDIGRPNUM       (0x2f0u) /* newest num of DIGRP */
#define OFFSET_NEWANSMPNUM_BASE  (0x2f2u) /* newest num of ANSMP  2Byte 1U and 1D */
/* yc board1 up AD data */
#define OFFSET_ANSMP1            (0x300u) /* 1 group AD data,board1,up AD channel from 1 to 7 every channel, 2Byte last is time */
#define OFFSET_ANSMP2            (0x310u) /* 2 group AD data,board1,up AD channel from 1 to 7 every channel, 2Byte last is time */
#define OFFSET_ANSMP3            (0x320u) /* 3 group AD data,board1,up AD channel from 1 to 7 every channel, 2Byte last is time */
#define OFFSET_ANSMP4            (0x330u) /* 4 group AD data,board1,up AD channel from 1 to 7 every channel, 2Byte last is time */
/* yc board1 - board4 AD data */
#define OFFSET_ANGRP1            (0x300u) /* board1 up AD data 32 group 32*16Byte */
#define OFFSET_ANGRP2            (0x500u) /* board1 up AD data 32 group 32*16Byte */
#define OFFSET_ANGRP3            (0x700u) /* board1 up AD data 32 group 32*16Byte */
#define OFFSET_ANGRP4            (0x900u) /* board1 up AD data 32 group 32*16Byte */
#define OFFSET_ANGRP5            (0xb00u) /* board1 up AD data 32 group 32*16Byte */
#define OFFSET_ANGRP6            (0xd00u) /* board1 up AD data 32 group 32*16Byte */
#define OFFSET_ANGRP7            (0xf00u) /* board1 up AD data 32 group 32*16Byte */
#define OFFSET_ANGRP8            (0x1100u)/* board1 up AD data 32 group 32*16Byte */
/* yc status analyze */
#define NONEBOARD                 	0		/* none board ready */
#define BOARD_1UP                 	1       /* get board 1 up AD's yc data */
#define BOARD_1DOWN				2       /* get board 1 down AD's yc data */
#define BOARD_2UP                	4       /* get board 2 up AD's yc data */
#define BOARD_2DOWN           		8       /* get board 2 down AD's yc data */
#define BOARD_3UP                  16      /* get board 3 up AD's yc data */
#define BOARD_3DOWN                32      /* get board 3 down AD's yc data */
#define BOARD_4UP                  64      /* get board 4 up AD's yc data */
#define BOARD_4DOWN                128     /* get board 4 down AD's yc data */
/* CMD out define */
#define EMIF_CMDOUT_ON				3		/* close */
#define EMIF_CMDOUT_OFF			0		/* open */
/* step-by-step num */
typedef enum
{
	STEP1 = 0,
	STEP2 = 1,
	STEP3 = 2,
	STEP4 = 3,
	STEP5 = 4,
	STEP6 = 5,
	STEP7 = 6,
	STEP8 = 7,
	STEP9 = 8,
	STEP10 = 9,
	STEP11 = 10,
	STEP12 = 11,
	STEP13 = 12,
	STEP14 = 13,
	STEP15 = 14,
	STEP16 = 15,
	STEP17 = 16,
	STEP18 = 17,
	STEP19 = 18,
	STEP20 = 19,
	STEP21 = 20,
	STEP22 = 21,
	STEP23 = 22,
	STEP24 = 23,
	STEP25 = 24,
	STEP26 = 25,
	STEP27 = 26,
	STEP28 = 27,
	STEP29 = 28,
	STEP30 = 29,
	STEP31 = 30,
	STEP32 = 31
}step_by_step;
/* Dout Cmd structure */
typedef struct _DoutCMDStru_
{
#ifdef DEBUG_RESERVE
	UInt32    Stuff:18;
	UInt32    Stuff2:4;
	UInt32    RelayNo:8;
	UInt32    OnOff:2;
#else
	UInt32    DoutFlag;				//
	UInt32	  DoutCMD1;
	UInt32    DoutCMD2;	
#endif
}DoutCMDStru;
/* base status structure */
typedef struct _BaseStatu_
{
	UChar       VersionID[6];      //version ID
	UChar       AnaRdy;            //ģ����׼����־
	UChar       DinRdy;            //������׼����־
	UInt32      Reserve;           //Ԥ����
	UInt32      Microsnd;          //΢�����
	UInt32      Dout1;             //�̵�����1�ֺ�״̬ 00 �� 11 ��
	UInt32      Dout2;             //�̵�����2�ֺ�״̬ 00 �� 11 ��
#ifdef DEBUG_RESERVE
	DoutCMDStru DoutCMD;           //�̵����ֺ��������
#else 
	UInt32      DoutCMD;
#endif
	UInt32      Ver;               //�汾��
	UInt32      ModeFpga;          //����ģʽ
}BaseStatu_Struct;
/* AD 7 channel data  */
typedef struct _ANSMP_
{
   Short sample1;                //AD ͨ��1����ֵ
   Short sample2;
   Short sample3;
   Short sample4;
   Short sample5;
   Short sample6;
   Short sample7;
   UInt16 MicrosndL;              //΢��ֵ�ĵ�λ
}ANSMP_Stru;
/* yx data struct */
typedef struct _DIGRP_
{
	UInt32   DIN1;        /* board1 input status */
	UInt32   DIN2;        /* board2 input status */
    UInt32   DIN3;        /* board3 input status */
	UInt32   diStamp;     /* yx timestamp microsecond */
}DIGRP_Struct;
/* yc ����32������ ��ÿ��ͨ��64�����ݷ�Ϊ���� 1bit ��ʾ�����������Ǹ�Ϊ��������          */
/* 0 ��ʾ�²�������Ϊ���� 1 ��ʾ�ϲ�������Ϊ���� ��32������+��32���������һ�����ڵ����� */
/* ����ʱӦ��ע�����64�����ݵ�˳�� ���������Ĵ���ʽ�õ�����ֵ                         */
typedef struct _VALIDPART_
{
	UChar board_1u:1;
	UChar board_1d:1;
	UChar board_2u:1;
	UChar board_2d:1;
	UChar board_3u:1;
	UChar board_3d:1;
	UChar board_4u:1;
	UChar board_4d:1;
}ValidPartFlag;
/* emif data structure */
typedef struct _Dataaddr_
{
	UInt16    dataInStatus;          /* emif yx yc ready status */
	UInt16    digitInaddr;           /* yx input offset address */
	UInt16    digitnum;              /* yx input length */
	UInt16    *digitOutaddr;         /* yx buffer address pointer */
	UInt16    analogyGlength;        /* yc input group length */
	UInt16    analogyG32length;      /* yc input all group length */
    UInt16    analogyInaddr1U;       /* yc input offset address board 1U(up) AD */
    UInt16    *analogyOutaddr1U;     /* yc buffer address pointer board 1U(up) AD */
}Dataaddr_Struct;
/* queue msg struct */
struct _MsgObj_
{
	Queue_Elem elem; 				/* first field for Queue */
	UChar AnaRdyStatus; 			/* ģ����׼����״̬ */
	UChar Flag;						/* ����������32��������Ч״̬ */
};
typedef struct _MsgObj_ AnaQueueMessStru;
/******************************* declar data ***********************************/
extern volatile BaseStatu_Struct FPGAReadStatus;		   //FPGA״̬��Ϣ
//extern volatile DIGRP_Struct DigInData;                  //����������
extern DIGRP_Struct DigInData;
extern ANSMP_Stru ANSInData[8][64];             //ң����ԭʼ���� 8*64 ��ʾ 8��AD ��64 ������(8ͨ������)
extern volatile ValidPartFlag ValidFlag;                 //�����ϲ��ֻ��²���Ϊ��������(ÿ��FPGAֻ��32�������)
extern Queue *AnaQueue;							 //ң�����
/******************************* functions **************************************/
Void EMIFA_Task(UArg arg0, UArg arg1);
Void EMIFA_ReadStatus(BaseStatu_Struct *statusptr);
Void EMIFA_ReadDigIn(UInt16* diginbuff);
Void EMIFA_ReadAnaInBoard(UInt32 baseaddr, Short* buffaddr);
Void EMIFA_ReadAnaIn(UChar analogNum);
Void EMIFA_WriteDisable(DoutCMDStru* cmdout);
Void EMIFA_WriteEnable(DoutCMDStru* cmdout);
UInt16 EMIFA_WriteDout(DoutCMDStru *cmdout);
Void EMIFA_ReadDout(UInt16 *readoutbuff);
UInt16 EMIFA_WriteCMDDout(DoutCMDStru *cmdout);
Void EMIFA_WriteCMD(DoutCMDStru *cmdout);

#if defined (__cplusplus)
}
#endif /* _EMIF_FPGA_H_ */
#endif /* _EMIF_FPGA_H_ */

