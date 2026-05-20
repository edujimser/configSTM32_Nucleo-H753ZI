/* ErrorHandler.h */
#ifndef __ERRORHANDLER_H
#define __ERRORHANDLER_H

// 1. Librerías estándar
#include <stdint.h>
#include <stdio.h>

// 2. Hardware básico de ST y acceso a los LEDs
#include "stm32h7xx_hal.h"
#include "LedDataVision/LedData.h"

typedef enum {
    ERR_PRUEBA = 0x00,                
    ERR_OSC_PLL = 0x01,               
    ERR_FLASH_LATENCY = 0x02,         
    ERR_RTC_INIT = 0x04,              
    ERR_UART3_INIT = 0x05,            
    ERR_UART3_TX_FIFO = 0x06,         
    ERR_UART3_RX_FIFO = 0x07,         
    ERR_UART3_FIFO_DISABLE = 0x08,    
    ERR_UART3_CLK_CONFIG = 0x09,      
    ERR_HANDLER_HARD = 0X0A,          
    ERR_HANDLER_USAGEFAULT = 0X0B,
    ERR_HANDLER_BUSFAULT = 0X0C,
    ERR_HANDLER_MEMMANAGE = 0X0D
} ErrorCode_t;

typedef struct {
    const char *file;        
    const char *function;    
    uint32_t    line;        
    uint32_t    err_code;    
    const char *err_name;    
    const char *description; 
    uint32_t   timestamp;    
} ErrorHandler_t; 

extern ErrorHandler_t ErrorHandler; 

/* Funciones del Manejador de Errores */
void Error_Handler_Init(void);
__attribute__((naked)) void Error_Handler(void);
__attribute__((naked)) void HardFault_Handler(void);
__attribute__((naked)) void UsageFault_Handler(void);
__attribute__((naked)) void BusFault_Handler(void);
__attribute__((naked)) void MemManage_Handler(void);

#endif /* __ERRORHANDLER_H */