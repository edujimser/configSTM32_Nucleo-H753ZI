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
// DEFINICIONES DE TRANSMISIÓN UART
// ==========================================================================================

#define LONGITUD_MAX_UART_TX            256
#define UART_QUEUE_TIMEOUT_TICKS        50

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
void MSG_Enviar(const char *texto, I_TestUart TestUart);
void MSG_Enviar_Test(const char *texto);
void MSG_Enviar_Error(const char *texto);
char MSG_Enviar_Longitud_Limit(const char *texto);
char MSG_Enviar_Caract_Valid(const char *texto);


#endif /* TXUART_H */
