#ifndef TXUART_H
#define TXUART_H

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "queue.h"
#include "task.h"
#include <stdbool.h>
#include <stdio.h>
#include "main.h"
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
#define LONG_TYPE_TEXT                  (LONG_MAX_UART_TX - LONG_TYPE_COMAND)
#define UART_QUEUE_TIMEOUT_TICKS        80
#define UART_STATE_LIST_HISTORY         30
#define UART_TX_MARGEN_MS               50
#define UART_STACK_SIZE_UART_TX         (256 * 8)
#define UART_QUEUE_UART_TX_SIZE        15U
// ==========================================================================================
// TRANSMISION UART
// ===================================================================================
typedef struct {
    char *Ptr;
    size_t len;
}S_UartMsg;

// ==========================================================================================
// STATUS UART
// ===================================================================================
typedef struct {
	osThreadId_t Thread_ID;
    uint32_t Thread_Stack_Uart_TX;
    uint32_t Thread_Stack_Uart_TX_Use;
    uint32_t Thread_Stack_Uart_TX_Free;
    uint32_t Thread_Heartbeat_Uart_TX;
    uint32_t Thread_Ticks_T0;
    uint32_t Thread_Ticks_Last;
    uint32_t Thread_Ticks_Uptime;

} S_UartMsgThread;

typedef struct {
    uint32_t Msg_Queue;          
    uint32_t Msg_Queue_Use;
    uint32_t Msg_Queue_Free;
    uint32_t Msg_Queue_Use_Max;
    uint32_t Msg_Queue_Invalid;
    uint32_t Msg_Queue_NoLen;
    uint32_t Msg_Queue_discarded;

} S_UartMsgQueue;

typedef struct {
	uint32_t Msg_GetState_Count_Ready;
	uint32_t Msg_GetState_Count_Busy;
	uint32_t Msg_GetState_Count_Timeout;
	uint32_t Msg_GetState_Count_Error;
	uint16_t Msg_GetSate_Count_List_Position;
	const char *   Msg_GetState_List_history[UART_STATE_LIST_HISTORY];
}S_GetState;

typedef struct {
	uint32_t Msg_TX_Ok;
	uint32_t Msg_TX_Timeout;
	uint32_t Msg_TX_Error;
	uint32_t Msg_TX_LenBytes;
	uint32_t Msg_TX_LastTime;
	uint32_t Msg_TX_TimeMax;
	uint32_t Msg_TX_Recovery;
	uint32_t Msg_TX_Busy;
	uint32_t Msg_TX_Discarded;
	uint32_t Msg_TX_Recovery_AbortFail;
	uint32_t Msg_TX_Recovery_LastTick;
	uint32_t Msg_TX_Recovery_FailsContinues;

}S_TX;

typedef struct {
    S_UartMsgThread Thread;
    S_UartMsgQueue  Queue;
    S_GetState      State;
    S_TX            TX;
} S_StatusUartMsg;

extern volatile S_StatusUartMsg StatusUartMsg;


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
static void MSG_UpdateStatusStatic(void);
static void MSG_UpdateStatusDinamic(void);
static void MSG_UpdateStatusState(HAL_UART_StateTypeDef st);

#endif /* TXUART_H */
