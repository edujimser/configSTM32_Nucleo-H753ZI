#ifndef TXUART_H
#define TXUART_H

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "queue.h"
#include "task.h"
#include <stdbool.h>
#include <stdio.h>
#include "MsgUart/TestUart/TestUart.h"

// ==========================================================================================
// CONFIGURACION TYPE MSG
// ==========================================================================================
typedef enum {
    MSG_GENERAL,
    MSG_FLASH_1,
    MSG_WEB,
    MSG_ERROR,
    TYPE_MSG_UART_COUNT
} E_TYPE_MSG_UART;

// ==========================================================================================
// DEFINICIONES DE TRANSMISIÓN UART
// ==========================================================================================
#define LONG_MAX_UART_TX                250
#define LONG_TYPE_COMAND                6
#define LONG_TYPE_TEXT                  LONG_MAX_UART_TX - LONG_TYPE_COMAND
#define UART_QUEUE_TIMEOUT_TICKS        80




typedef struct {
    char *Ptr;
    size_t len;
}S_UartMsg;

extern uint32_t Stack_libre_uarth;
extern uint32_t Stack_libre_uarth_critic;
extern osMessageQueueId_t Queue_Uart_Tx;


// ==========================================================================================
// PROTOTIPOS DE FUNCIONES PÚBLICAS
// ==========================================================================================
void ThreadFunc_Uart_TX(void *argument);
void MSG_Enviar(E_TYPE_MSG_UART typeMSG, const char *texto, I_TestUart TestUart);
void MSG_Enviar_Test(const char *texto);
void MSG_Enviar_Error(const char *texto);
char MSG_Enviar_Longitud_Limit(const char *texto);
char MSG_Enviar_Longitud_LimitBuf(const char *texto);
char MSG_Enviar_Caract_Valid(const char *texto);


#endif /* TXUART_H */
