#ifndef RXMSGUART_H
#define RXMSGUART_H

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "queue.h"
#include "task.h"
#include <stdbool.h>
#include <stdio.h>
#include "MsgUart/TestUart/TestUart.h"

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
#endif /* RXMSGUART_H */
