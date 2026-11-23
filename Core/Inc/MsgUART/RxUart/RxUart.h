#ifndef RXMSGUART_H
#define RXMSGUART_H

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "queue.h"
#include "task.h"
#include <stdbool.h>
#include <stdio.h>
#include "MsgUart/TestUart/TestUart.h"

#define LONG_TYPE_TEXT_RX 128


extern osMessageQueueId_t Queue_Uart_Rx;
extern osThreadId_t UartRXTaskHandle;
extern uint8_t rxDmaBuffer[LONG_TYPE_TEXT_RX];
extern uint8_t processingBuffer[LONG_TYPE_TEXT_RX];
extern uint16_t receivedDataSize;

typedef struct {
    char *Ptr;
    size_t len;
}S_UartRXMsg;



void ThreadFunc_Uart_Rx(void *argument);
#endif /* RXMSGUART_H */
