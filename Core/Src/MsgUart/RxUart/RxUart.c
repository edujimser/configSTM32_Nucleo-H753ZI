#include "MsgUart/TxUart/TxUart.h"
#include <string.h>
#include "main.h"
#include <stdint.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "usart.h"
#include "stm32h7xx_hal_uart.h"
#include "MsgUart/TestUart/TestUart.h"
#include "MsgUart/RxUart/RxUart.h"

osMessageQueueId_t hUartQueueRX = NULL;
osThreadId_t UartRXTaskHandle = NULL;
ALIGN_32BYTES(uint8_t rxDmaBuffer[LONGITUD_MAX_UART_RX]);
ALIGN_32BYTES(uint8_t processingBuffer[LONGITUD_MAX_UART_RX]);
uint16_t receivedDataSize = 0;
extern I_TestUart TestUart_1;


void vTaskUartRxHandler (void *argument)
{
    vTaskDelay(pdMS_TO_TICKS(100));

    if (huart3.hdmarx != NULL) {
        HAL_UARTEx_ReceiveToIdle_DMA(&huart3, rxDmaBuffer, LONGITUD_MAX_UART_RX);
        __HAL_DMA_DISABLE_IT(huart3.hdmarx, DMA_IT_HT);
    }

    for (;;) {
        // Esperamos a que la ISR nos notifique que hay datos nuevos (bloqueo eficiente indefinido)
        uint32_t notifiedValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (notifiedValue > 0 && receivedDataSize > 0) {
            // Ya tenemos los datos seguros en 'processingBuffer' y su tamaño en 'receivedDataSize'

            if (receivedDataSize >= 4) {

                if (strncmp((char *)processingBuffer, "PRUEBA_TX", 9) == 0) {
                	Prueba_TX(TestUart_1, true);
                }
                else {
                    MSG_Enviar("Comando no reconocido\r\n", TestUart_1);
                }
            }

            // Limpiamos por seguridad
            receivedDataSize = 0;
        }
    }
}
