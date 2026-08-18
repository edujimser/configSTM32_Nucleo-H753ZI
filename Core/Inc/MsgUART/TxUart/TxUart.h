#ifndef MSGUART_H
#define MSGUART_H

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "queue.h"
#include "task.h"
#include <stdbool.h>
#include <stdio.h>
// ==========================================================================================================================================
// TRANSMISIÓNM
// ==========================================================================================================================================

#define LONGITUD_MAX_UART               125
#define UART_QUEUE_TIMEOUT_TICKS        50

typedef struct {
    char *Ptr;
    size_t len;
}S_UartMsg;

extern uint32_t Stack_libre_uarth;
extern uint32_t Stack_libre_uarth_critic;

extern osMessageQueueId_t hUartQueue;
extern osThreadId_t UartTaskHandle;

void vTaskUartHandler(void *argument);
__attribute__((noinline)) void MSG_Enviar(const char *texto);
void MSG_Enviar_Error(const char *texto);
char MSG_Enviar_Longitud_Limit(const char *texto);
char MSG_Enviar_Caract_Valid(const char *texto);

// PRUEBAS INDIVIDUALES ======================================================================================================================
extern bool pruebasEjecutadas;

void Prueba_TX(void);
void Prueba_Longitud_Maxima(void);
void Prueba_Mensajes_Vacios(void);
void Prueba_Caracteres_Invalidos(void);
void Prueba_Saturacion_Cola(void);
void PruebaConcurrente_Tiempo(uint32_t tiempo_ms);
void Prueba_Abortos_DMA_Repetidos(void);
void Prueba_FIFO_Corrupta(void);
void Prueba_Puntero_Reutilizado(void);
#endif /* MSGUART_H */


// ==========================================================================================================================================
// RECEPCION
// ==========================================================================================================================================
#define LONGITUD_MAX_UART_RX 128

extern osMessageQueueId_t hUartQueueRX;
extern osThreadId_t UartRXTaskHandle;
extern uint8_t rxDmaBuffer[LONGITUD_MAX_UART_RX];
extern uint8_t processingBuffer[LONGITUD_MAX_UART_RX];
extern uint16_t receivedDataSize;

typedef struct {
    char *Ptr;
    size_t len;
}S_UartRXMsg;


void vTaskUartRxHandler(void *argument);
