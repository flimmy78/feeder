#ifndef _LOG_H_
#define _LOG_H_

/* 0: printf; 1: UART */
#define OUTPUT_TO_UART1 1

/* Switch Log Output */
#if OUTPUT_TO_UART1
    /* For UART1 Initial */
    #include <stdlib.h>
    #include "hw_types.h"               
    #include "hw_syscfg0_C6748.h"      
    #include "soc_C6748.h"            
    #include "psc.h"                 
    #include "gpio.h"               
    #include "uart.h"              
    #include "uartStdio.h"

    #define OUTPUT UARTprintf
#else
    #include <stdio.h>
    #include <stdbool.h>

    #define OUTPUT printf
#endif

/* Log init */
#if OUTPUT_TO_UART1
    #define LOG_INIT() do {                                     \
        PSCModuleControl(SOC_PSC_1_REGS, HW_PSC_GPIO,           \
                         PSC_POWERDOMAIN_ALWAYS_ON,             \
                         PSC_MDCTL_NEXT_ENABLE);                \
        GPIOBank6Pin12PinMuxSetup();                            \
        GPIOBank6Pin13PinMuxSetup();                            \
        GPIODirModeSet(SOC_GPIO_0_REGS, 109, GPIO_DIR_OUTPUT);  \
        GPIODirModeSet(SOC_GPIO_0_REGS, 110, GPIO_DIR_OUTPUT);  \
        UARTStdioInit();                                        \
    } while (0)
#else
    #define LOG_INIT() do { } while (0)
#endif

/* Log Output */
#if OUTPUT_TO_UART1
#define LOG_INFO(format, ...)      \
    do {                           \
        OUTPUT("[%s|%s@%s,%d] " format "\n", "INFO ", __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
    } while (0)
#else
    #define LOG_INFO(format, ...) do { } while (0)
#endif
//#define LOG_ERROR(format, ...)     \
//    do {                           \
//        OUTPUT("[%s|%s@%s,%d] " format "\n", "ERROR", __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
//    } while (0)

//#define LOG_DEBUG(format, ...)     \
//    do {                           \
//        OUTPUT("[%s|%s@%s,%d] " format "\n", "DEBUG", __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
//    } while (0)

//#define LOG_WARN(format, ...)      \
//    do {                           \
//        OUTPUT("[%s|%s@%s,%d] " format "\n", "WRAN ", __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
//    } while (0)

//#define LOG_TRACE(format, ...)     \
//    do {                           \
//        OUTPUT("[%s|%s@%s,%d] " format "\n", "TRACE", __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
//    } while (0)

//#define LOG_FATAL(format, ...)     \
//    do {                           \
//        OUTPUT("[%s|%s@%s,%d] " format "\n", "FATAL", __func__, __FILE__, __LINE__, ##__VA_ARGS__ ); \
//        exit(1);                   \
//    } while (0)

/* Assert */
#define ASSERT(EXP)                \
    do{                            \
        if (!(EXP)) {              \
            OUTPUT("[%s@%s,%d] ASSERT: " #EXP "\n", __func__, __FILE__, __LINE__ );                  \
            exit(1);               \
        }                          \
    } while(0)

#endif

