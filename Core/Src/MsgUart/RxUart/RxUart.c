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
#include "SPI_Flash_Memory/SPI_Flash_Memory.h"

osMessageQueueId_t Queue_Uart_Rx = NULL;
ALIGN_32BYTES(uint8_t rxDmaBuffer[LONGITUD_MAX_UART_RX]);
ALIGN_32BYTES(uint8_t processingBuffer[LONGITUD_MAX_UART_RX]);
uint16_t receivedDataSize = 0;
extern I_TestUart TestUart_1;


extern osThreadId_t ThreadId_InitTask;
extern osThreadId_t ThreadId_Test;
extern osThreadId_t ThreadId_StatusSystem;
extern osThreadId_t ThreadId_Uart_Tx;
extern osThreadId_t ThreadId_Uart_Rx;
extern osThreadId_t defaultTaskHandle;


void ThreadFunc_Uart_Rx (void *argument)
{
    vTaskDelay(pdMS_TO_TICKS(100));

    if (huart3.hdmarx != NULL) {
        HAL_UARTEx_ReceiveToIdle_DMA(&huart3, rxDmaBuffer, LONGITUD_MAX_UART_RX);
        __HAL_DMA_DISABLE_IT(huart3.hdmarx, DMA_IT_HT);
    }

    for (;;) {
        uint32_t notifiedValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (notifiedValue > 0 && receivedDataSize > 0) {

        	if (strncmp((char *)processingBuffer, "PRUEBA_TX", 9) == 0)
        	{
        		Prueba_TX(TestUart_1, true);
        	}
        	else if (strncmp((char *)processingBuffer, "FLASH_ID", 8) == 0)
        	{
        		Flash_Read_ID(&spi_flash_memory_1);
        	}
        	else if (strncmp((char *)processingBuffer, "FLASH_COMPONENT", 15) == 0)
			{
        		Flash_Read_Component(&spi_flash_memory_1);
			}
        	else if (strncmp((char *)processingBuffer, "FLASH_ERRARSE_ALL", 17) == 0)
			{
				vTaskSuspend(ThreadId_Test);
				vTaskSuspend(defaultTaskHandle);

        		Flash_Erase_All(&spi_flash_memory_1);


        		vTaskResume(ThreadId_Test);
        		vTaskResume(defaultTaskHandle);
			}
        	else if (strncmp((char *)processingBuffer, "FLASH_ERARSE_SECTOR", 19) == 0)
			{

        		vTaskSuspend(ThreadId_Test);
        		vTaskSuspend(defaultTaskHandle);

        		Flash_Erase_All_For_Sector(&spi_flash_memory_1, 4096);


        		vTaskResume(ThreadId_Test);
        		vTaskResume(defaultTaskHandle);
			}
        	else if (strncmp((char *)processingBuffer, "FLASH_WRITE_SECTOR", 18) == 0)
			{

        		vTaskSuspend(ThreadId_Test);
        		vTaskSuspend(defaultTaskHandle);

        		Flash_Write_All_Sectors_Data(&spi_flash_memory_1, 4096, 16, 0xAA);

        		vTaskResume(ThreadId_Test);
        		vTaskResume(defaultTaskHandle);
			}
        	else if (strncmp((char *)processingBuffer, "FLASH_WRITER_SECTOR_0", 21) == 0)
			{

        		vTaskSuspend(ThreadId_Test);
        		vTaskSuspend(defaultTaskHandle);


				uint8_t pequeño_buffer[64];
				memset(pequeño_buffer, 0xFF, sizeof(pequeño_buffer));
				const char *mi_mensaje = "Hola Garaje 49 - Dato corto";
				memcpy(pequeño_buffer, mi_mensaje, strlen(mi_mensaje));


        		Flash_Write_One_Sector(&spi_flash_memory_1, PART_CONFIG, GET_SECTOR_ADDR(0), pequeño_buffer, sizeof(pequeño_buffer));


        		vTaskResume(ThreadId_Test);
        		vTaskResume(defaultTaskHandle);
			}
        	else if (strncmp((char *)processingBuffer, "Flash_Dump_All_Sectors", sizeof("Flash_Dump_All_Sectors") - 1) == 0)
			{

				vTaskSuspend(ThreadId_Test);
				vTaskSuspend(defaultTaskHandle);

				Flash_Dump_All_Sectors(&spi_flash_memory_1);

				vTaskResume(ThreadId_Test);

			}
        	else if (strncmp((char *)processingBuffer, "Flash_Dump_One_Sectors_Hex", sizeof("Flash_Dump_One_Sectors_Hex") - 1) == 0)
			{
				char *puntero_numeros = (char *)processingBuffer + 26;
				uint32_t sector_temp = 0;
				uint32_t page_temp = 0;

				// Example Flash_Dump_One_Page_Hex 10 3
				if (sscanf(puntero_numeros, "%lu", &sector_temp) == 1)
				{
					vTaskSuspend(ThreadId_Test);
					vTaskSuspend(defaultTaskHandle);


					Flash_Dump_One_Sectors_Hex(&spi_flash_memory_1, sector_temp);

					vTaskResume(ThreadId_Test);
					vTaskResume(defaultTaskHandle);
			   }

			}
        	else if (strncmp((char *)processingBuffer, "Flash_Dump_One_Sectors_ASCII", sizeof("Flash_Dump_One_Sectors_ASCII") - 1) == 0)
			{
				char *puntero_numeros = (char *)processingBuffer + 28;
				uint32_t sector_temp = 0;
				uint32_t page_temp = 0;

				// Example Flash_Dump_One_Page_Hex 10 3
				if (sscanf(puntero_numeros, "%lu", &sector_temp) == 1)
				{
					vTaskSuspend(ThreadId_Test);
					vTaskSuspend(defaultTaskHandle);


					Flash_Dump_One_Sectors_ASCII(&spi_flash_memory_1, sector_temp);

					vTaskResume(ThreadId_Test);
					vTaskResume(defaultTaskHandle);
			   }

			}
        	else if (strncmp((char *)processingBuffer, "Flash_Dump_One_Page_Hex", sizeof("Flash_Dump_One_Page_Hex") - 1) == 0)
			{
				char *puntero_numeros = (char *)processingBuffer + 24;
				uint32_t sector_temp = 0;
				uint32_t page_temp = 0;

				// Example Flash_Dump_One_Page_Hex 10 3
				if (sscanf(puntero_numeros, "%lu %lu", &sector_temp, &page_temp) == 2)
				{
					vTaskSuspend(ThreadId_Test);
					vTaskSuspend(defaultTaskHandle);


					Flash_Dump_One_Page_Hex(&spi_flash_memory_1, sector_temp, page_temp);

					vTaskResume(ThreadId_Test);
					vTaskResume(defaultTaskHandle);
			   }

			}
        	else if (strncmp((char *)processingBuffer, "Flash_Dump_One_Page_ASCII", sizeof("Flash_Dump_One_Page_ASCII") - 1) == 0)
        	{
        		char *puntero_numeros = (char *)processingBuffer + 26;
        		uint32_t sector_temp = 0;
        		uint32_t page_temp = 0;

        	    // Example Flash_Dump_One_Page_ASCII 10 3
        		if (sscanf(puntero_numeros, "%lu %lu", &sector_temp, &page_temp) == 2)
        	    {
					vTaskSuspend(ThreadId_Test);
					vTaskSuspend(defaultTaskHandle);


					Flash_Dump_One_Page_ASCII(&spi_flash_memory_1, sector_temp, page_temp);

					vTaskResume(ThreadId_Test);
					vTaskResume(defaultTaskHandle);
        	   }

        	}
        	else
        	{
				MSG_Enviar("COMANDO DESCONOCIDO EN RX", TestUart_1);
        	}

            // Limpiamos por seguridad
            receivedDataSize = 0;
        }
    }
}
