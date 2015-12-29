/*
 * Copyright (c) 2012, Texas Instruments Incorporated
 * All rights reserved.
 *
 * Redistribution and use in source and binary forms, with or without
 * modification, are permitted provided that the following conditions
 * are met:
 *
 * *  Redistributions of source code must retain the above copyright
 *    notice, this list of conditions and the following disclaimer.
 *
 * *  Redistributions in binary form must reproduce the above copyright
 *    notice, this list of conditions and the following disclaimer in the
 *    documentation and/or other materials provided with the distribution.
 *
 * *  Neither the name of Texas Instruments Incorporated nor the names of
 *    its contributors may be used to endorse or promote products derived
 *    from this software without specific prior written permission.
 *
 * THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS "AS IS"
 * AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT LIMITED TO,
 * THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR
 * PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT OWNER OR
 * CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL,
 * EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO,
 * PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR PROFITS;
 * OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY,
 * WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR
 * OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE,
 * EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
 */

/*
 *  ======== AppCommon.h ========
 *
 */

#ifndef AppCommon__include
#define AppCommon__include
#if defined (__cplusplus)
extern "C" {
#endif


/*
 *  ======== Application Configuration ========
 */

#define SHARED_REGION_0         0x0
#define SHARED_REGION_1         0x1

#define SHAREAREA_SIZE		0x4000  
#define SystemCfg_LineId        0
#define SystemCfg_EventId       7

#define App_CMD_NOP                    	0x10000000   /* cc------ */
#define App_E_FAILURE                   	0xF0000000
#define App_CMD_RESRDY              	0x09000000  /* cc-------*/
#define App_CMD_READY                 	0x0A000000  /* cc-------*/
#define App_CMD_SDACK                 	0x0B000000  /* cc-------*/
#define App_CMD_CLOSED                	0x0C000000  /* cc-------*/
#define App_CMD_DATAREQ             	0x0D000000  /* cc-------*/
#define App_CMD_SHUTDOWN_ACK  	0x30000000  /* cc------ */
#define App_CMD_SHUTDOWN          	0x31000000  /* cc------ */
#define App_CMD_OP_COMPLETE     	0x11000000  /* cc------ */
#define App_E_OVERFLOW               	0xF1000000
#define App_E_FAILURE                   	0xF0000000
#define App_SPTR_HADDR               	0x20000000  /* cc--hhhh */
#define App_SPTR_LADDR                	0x21000000  /* cc--llll */
#define App_SPTR_ADDR_ACK          	0x22000000  /* cc------ */
#define App_SPTR_MASK                   	0x0000FFFF  /* ----pppp */

#define App_E_FAILURE                    	0xF0000000
#define App_E_OVERFLOW                	0xF1000000

#define DSP_CMD_PRINTF					0x1102	   //MessageQ消息，用于DSP的打印
#define DSP_CMD_POWERDOWN				0x1103

#define SERVER_MsgHeapName   "SerMsgHeap:0"
#define App_MsgHeapName         "MsgHeap:0"
#define App_MsgHeapSrId            0
#define App_MsgHeapId               0
#define App_HostMsgQueName      "HOST:MsgQ:0"
#define App_VideoMsgQueName     "VIDEO:MsgQ:0"

#if defined (__cplusplus)
}
#endif /* defined (__cplusplus) */
#endif /* AppCommon__include */
