/*************************************************************************/
// iec104.h                                        Version 1.00
//
// ±¾ÎÄ¼þÊÇDTU2.0Í¨Ñ¶Íø¹Ø×°ÖÃµÄIEC60870-5-104¹æÔ¼´¦Àí³ÌÐò
// ±àÐ´ÈË  :R&N
// email		  :R&N
//  ÈÕ	   ÆÚ:2015.04.17
//  ×¢	   ÊÍ:
/*************************************************************************/
#ifndef    _IEC104_H
#define    _IEC104_H


#define IEC104CONFFILE				"/config/guiyue104.conf"
#define IEC104LOGFFILE				"/config/log104.txt"
#define IEC104SOELOG                "/config/SOE104.txt"
#define IEC104SOEFILE               "/config/soefile.txt"



#define IEC104FRAMEERR         		    1
#define IEC104FRAMESEQERR      		    2
#define IECFRAMENEEDACK        		    3
#define IEC104FRAMEOK          		    0
#define IEC104FRAME_I          			0x10
#define IEC104FRAME_S          			0x11
#define IEC104FRAME_U         		 	0x13
#define IEC104BUFDOWNNUMBER		        4	//·ÖÅä6¸ö»º³å 6*256
#define IEC104BUFDOWNLENGTH		        256
#define IEC104BUFUPLENGTH			    4
#define IEC104YXINFONUMBER		        32	//·ÖÅä4¸ö»º³å
#define IEC104SOENUMBER		            32	//·ÖÅä4¸ö»º³å


#define FORMAT_I                		0
#define FORMAT_S                		1
#define FORMAT_U                		3

#define IEC104_M_SP_NA_1        		1
#define IEC104_M_ME_NA_1        		9	//²âÁ¿Öµ£¬¹éÒ»»¯Öµ
#define IEC104_M_ME_NC_1			    13  //²âÁ¿Öµ£¬¶Ì¸¡µãÊý
#define IEC104_M_IT_NA_1        		15
#define IEC104_M_SP_TB_1        		30
#define IEC104_M_ME_TD_1        		34
#define IEC104_M_IT_TB_1        		37
#define IEC104_C_DC_NA_1        		46
#define IEC104_C_SC_NA_1        		45
#define IEC104_C_SC_TA_1        		58
#define IEC104_C_IC_NA_1        		100
#define IEC104_C_CI_NA_1        		101
#define IEC104_C_RD             		102
#define IEC104_C_CS_NA_1        		103
#define IEC104_C_RP_NA_1        		105
#define IEC104_DC_NA_1    			    0X2E
#define IEC104_CAUSE_N_BIT              0x40

#define IEC104_CAUSE_SPONT                3
#define IEC104_CAUSE_REQ                  5
#define IEC104_CAUSE_ACT                  6
#define IEC104_CAUSE_ACTCON               7
#define IEC104_CAUSE_DEACT                8
#define IEC104_CAUSE_DEACTCON             9
#define IEC104_CAUSE_ACTTERM              10
#define IEC104_CAUSE_INTROGEN             20
#define IEC104_CAUSE_UNKNOWNTYPE          44
#define IEC104_CAUSE_UNKNOWNCAUSE         45
#define IEC104_CAUSE_UNKNOWNCOMMADDR      46
#define IEC104_CAUSE_UNKNOWNINFOADDR      47

#define IEC104WAITIDLETIME      		  50

//define SIQ_BIT
#define IEC104_SIQ_IV_BIT    			0x80
#define IEC104_SIQ_NT_BIT    		    0x40
#define IEC104_SIQ_SB_BIT    			0x20
#define IEC104_SIQ_BL_BIT    			0x10
#define IEC104_SIQ_OV_BIT   			0x01

//define QOI
#define IEC104_QOI_INTROGEN  		    0x14
#define IEC104_QOI_INTRO1    		    0x15
#define IEC104_QOI_INTRO2    		    0x16
#define IEC104_QOI_INTRO3    		    0x17
#define IEC104_QOI_INTRO4    		    0x18
#define IEC104_QOI_INTRO5    		    0x19
#define IEC104_QOI_INTRO6    		    0x1A
#define IEC104_QOI_INTRO7    		    0x1B
#define IEC104_QOI_INTRO8    		    0x1C
#define IEC104_QOI_INTRO9    		    0x1D
#define IEC104_QOI_INTRO10   		    0x1E
#define IEC104_QOI_INTRO11   		    0x1F
#define IEC104_QOI_INTRO12   		    0x20
#define IEC104_QOI_INTRO13   		    0x21
#define IEC104_QOI_INTRO14   		    0x22
#define IEC104_QOI_INTRO15   		    0x23
#define IEC104_QOI_INTRO16   		    0x24

//define RTU_VSQ_BIT
#define IEC104_VSQ_SQ_BIT    		0x80   //¼ì²âÊÇ·ñÊÇ²âÊÔ
#define IEC104_VSQ_PN_BIT    		0x40   //¼ì²âÊÇ·ñÊÇÁ¬Ðø

#define	IEC104_MAX_K			    12  //·¢ËÍ×´Ì¬±äÁ¿µÄ×î´ó²»Í¬µÄ½ÓÊÕÐòºÅ
#define	IEC104_MAX_W			    8    //½ÓÊÕw¸öI¸ñÊ½APDUsÖ®ºóµÄ×îºóµÄÈÏ¿É



#define TYPE_GUIYI                  9
#define TYPE_FLOAT                  11

/**************************ÏµÍ³ÐèÒªµÄÁ´±í½á¹¹Ìå******************************/
struct _BufStruct
{
	unsigned short usStatus;
	unsigned short usLength;
	unsigned char * buf;
	struct _BufStruct * next;
};
typedef struct _BufStruct BufStruct;

struct _IEC104Struct
{
	unsigned short  usRecvNum;  //ÒÑ¾­½ÓÊÕµ½µÄÖ¡
	unsigned short  usSendNum;  //ÒÑ¾­·¢ËÍ³öµÄÖ¡
	unsigned short  usServRecvNum; //·þÎñÆ÷½ÓÊÕµ½µÄÖ¡
	unsigned short  usServSendNum; //·þÎñÆ÷ÒÑ·¢ËÍµÄÖ¡
	unsigned short  usAckNum;   //ÒÑ¾­ÈÏ¿ÉµÄÖ¡

	unsigned char	 mode;			//mode=1#Ä£Ê½:²»Æ½ºâ(0),Æ½ºâ(1)
	unsigned char  linkaddr;		//linkaddr=2#Á´Â·µØÖ·
	unsigned char  linkaddrlen;		//linkaddrlen=2#Á´Â·µØÖ·³¤¶È
	unsigned char  conaddrlen;		//conaddrlen=2#¹«¹²µØÖ·³¤¶È
	unsigned char  COTlen;			//COTlen=2#´«ËÍÔ­Òò³¤¶È
	unsigned char  infoaddlen;		//infoaddlen=2#ÐÅÏ¢ÌåµØÖ·³¤¶È
	unsigned char  yctype;			//yctype=9#Ò£²âÀàÐÍ:¹æÒ»Öµ(9),±ê¶ÈÖµ(11),ÎÞÆ·ÖÊ¹æÒ»Öµ(21)
	unsigned char  dir;				//dir=1#Æ½ºâÄ£Ê½DIRÎ»(0»ò1,È±Ê¡0)

	unsigned char   ucSendCountOverturn_Flag;  //·¢ËÍ¼ÆÊý·­×ª±êÖ¾

	unsigned char   ucTimeOut_t0;  //Á¬½Ó½¨Á¢³¬Ê±Öåååµ,µ¥Î»s 60
	unsigned char   ucTimeOut_t1;  //APDUµÄ·¢ËÍ»ò²âÊÔµÄ³¬Ê±Ê±¼ä,s60
	unsigned char   ucTimeOut_t2;  //ÎÞÊý¾Ý±¨ÎÄt2<t1Çé¿öÏÂÈÏ¿ÉµÄ³¬Ê±Ê±¼ä,s20
	unsigned char   ucTimeOut_t3;  //ÜÝ³¤Ê±¼äIdle×´Ì¬t3>t1Çé¿öÏÂ·¢ËÍS-Ö¡µÄ³¬Ê±Ê±¼ä,s40

	unsigned char   k;            		 //·¢ËÍI¸ñÊ½Ó¦ÓÃ¹æÔ¼Êý¾Ýµ¥ÔªµÄÎ´ÈÏ¿ÉÖ¡Êý ×î´ó²î±ð½ÓÊÕÐòºÅÀ´·¢ËÍ×´Ì¬±äÁ¿
	unsigned char   w;            		 //½ÓÊÕI¸ñÊ½Ó¦ÓÃ¹æÔ¼Êý¾Ýµ¥ÔªµÄÖ¡Êý ½ÓÊÜµ½w I¸ñÊ½µÄAPDUºóµÄ×îÐÂÈ·ÈÏ


	unsigned char   ucDataSend_Flag;   //ÊÇ·ñÔÊÐí·¢ËÍ±êÖ¾

//	unsigned char   ucIdleCount_Flag;//ÊÇ·ñÔÊÐít2¼ÆÊý
//	unsigned char   ucIdleCount;        //t2¼ÆÊý
//
//	unsigned char   ucWaitServConCount_Flag;//ÊÇ·ñt1¼ÆÊý
//	unsigned char   ucWaitServConCount; //t1¼ÆÊý
//
//	unsigned char   ucNoRecvCount; //t0¼ÆÊý
//
	unsigned char ucYK_Limit_Time;
	unsigned char ucYK_TimeCount_Flag;
	unsigned char ucYK_TimeCount;

	unsigned char * pSendBuf;
	unsigned char ucInterro_First;
	int			logfd;
    
    int         logsoEfd;
    int         soEfile;

};
typedef	struct _IEC104Struct IEC104Struct;
struct _IEC104_ASDU_Struct
{
	unsigned char ucType;
	unsigned char ucRep_Num;      //¿É±ä½á¹¹ÏÞ¶¨´Ê
	unsigned char ucCauseL;        //´«ËÍÔ­Òò
	unsigned char ucCauseH;        //´«ËÍÔ­Òò
	unsigned char ucCommonAddrL;   //¹«¹²µØÖ·
	unsigned char ucCommonAddrH;   //¹«¹²µØÖ·
	unsigned char ucInfoUnit[243];//ÐÅÏ¢Ìåµ¥Ôª,length=249-6
};
typedef struct _IEC104_ASDU_Struct IEC104_ASDU_Frame;



/**********************extern function**********************************/
extern IEC104Struct  	* pIEC104_Struct;

extern void Init_Iec104_Logfile( void );
extern void  *Network_Thread( void * data  );
extern void  *Iec104_Thread( void * data  );
extern int  IEC104_Free_Buf(void);
extern int  IEC104_Init_Buf(void);
extern void IEC104_Process_Message(UInt32 cmd, UInt32 data, UInt32 buf);
extern void IEC104_Senddata2pc(int fd, char *tmp_buf, UInt8 len);
extern void SOE104_Senddata2pc(int fd, char *tmp_buf, UInt8 len);

extern void PC_Send_YC(UInt8* buf,UInt8 len);
extern void PC_Send_YX(UInt8* buf,UInt8 len);
extern void Send_Soe_Data(int fd);
extern void Get_Soe_From_Line(char * line);
extern unsigned long file_wc(int file);  
extern void sig_handler(int signo);
extern void set_timer();
extern void uninit_time();

#endif

