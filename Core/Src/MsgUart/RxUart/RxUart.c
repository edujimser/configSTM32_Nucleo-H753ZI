
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
#include "SPI_Flash_Memory/SPI_Flash_Sections/SPI_flash_section_log_monitor/SPI_Flash_Logs_Monitor.h"

osMessageQueueId_t Queue_Uart_Rx = NULL;
ALIGN_32BYTES(uint8_t rxDmaBuffer[LONG_TYPE_TEXT_RX]);
ALIGN_32BYTES(uint8_t processingBuffer[LONG_TYPE_TEXT_RX]);
uint16_t receivedDataSize = 0;
extern I_TestUart Uart1;


extern osThreadId_t ThreadId_InitTask;
extern osThreadId_t ThreadId_StatusSystem;
extern osThreadId_t ThreadId_Uart_Tx;
extern osThreadId_t ThreadId_Uart_Rx;
extern osThreadId_t defaultTaskHandle;


void ThreadFunc_Uart_Rx (void *argument)
{
    vTaskDelay(pdMS_TO_TICKS(100));

    if (huart3.hdmarx != NULL) {
        HAL_UARTEx_ReceiveToIdle_DMA(&huart3, rxDmaBuffer, LONG_TYPE_TEXT_RX);
        __HAL_DMA_DISABLE_IT(huart3.hdmarx, DMA_IT_HT);
    }

    for (;;) {
        uint32_t notifiedValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (notifiedValue > 0 && receivedDataSize > 0) {

    	    vTaskSuspend(defaultTaskHandle);

        	/* ==============================================================================
        	 * FLASH --->
        	 * ============================================================================== */


        	if (strncmp((char *)processingBuffer, "TEST_TX", 7) == 0) {
        	    Prueba_TX(Uart1, true);
        	}

        	/* ==============================================================================
        	 * FLASH READ --->
        	 * ============================================================================== */
        	else if (strncmp((char *)processingBuffer, "FLASH_READ_ALL_SECTOR_", 22) == 0) {

        	    char *sub_cmd = (char *)processingBuffer + 22;

        	    //FLASH_READ_ALL_SECTOR_HEX -->
        	    if (strncmp(sub_cmd, "HEX", 3) == 0) {
        	        Flash_CMD_UART_Read_All_Sectors(&spi_flash_memory_1, VISION_HEXADECIMAL);
        	    }
        	    //FLASH_READ_ALL_SECTOR_ASCII -->
        	    else if (strncmp(sub_cmd, "ASCII", 5) == 0) {
        	        Flash_CMD_UART_Read_All_Sectors(&spi_flash_memory_1, VISION_ASCII);
        	    }
        	    //FLASH_READ_ALL_SECTOR_DUAL -->
        	    else if (strncmp(sub_cmd, "DUAL", 4) == 0) {
        	        Flash_CMD_UART_Read_All_Sectors(&spi_flash_memory_1, VISION_HEXADECIMAL_AND_ASCII);
        	    }
        	    //FLASH_READ_ALL_SECTOR_DIR -->
        	    else if (strncmp(sub_cmd, "DIR ", 4) == 0) {
        	        char *puntero_numeros = sub_cmd + 4;
        	        int option_temp = 0;
        	        uint32_t sectorInit_temp = 0, sectorFinal_temp = 0;
        	        uint16_t numPage_temp;

        	        if (sscanf(puntero_numeros, "%d %lu %lu %hu", &option_temp, &sectorInit_temp, &sectorFinal_temp, &numPage_temp) == 4) {

        	        	numPage_temp = ((numPage_temp > GET_FLASH_NUM_PAGE) || (numPage_temp == 0)) ? GET_FLASH_NUM_PAGE : numPage_temp;
        	        	Flash_CMD_UART_Read_Range_Sectors_Page(&spi_flash_memory_1, (E_CMD_UART_Read)option_temp, sectorInit_temp, sectorFinal_temp, numPage_temp);
        	        }
        	    }

        	    //FLASH_READ_ALL_SECTOR_ENUM_HEX_LOAD -->
        	    else if (strncmp(sub_cmd, "ENUM_HEX_LOAD", 13) == 0) {
        	        Flash_CMD_UART_Read_EnumSector(&spi_flash_memory_1, VISION_HEXADECIMAL, FLASH_SECTION_LOAD);
        	    }
        	    //FLASH_READ_ALL_SECTOR_ENUM_ASCII_LOAD -->
        	    else if (strncmp(sub_cmd, "ENUM_ASCII_LOAD", 15) == 0) {
        	        Flash_CMD_UART_Read_EnumSector(&spi_flash_memory_1, VISION_ASCII, FLASH_SECTION_LOAD);
        	    }
        	    //FLASH_READ_ALL_SECTOR_ENUM_DUAL_LOAD -->
        	    else if (strncmp(sub_cmd, "ENUM_DUAL_LOAD", 14) == 0) {
        	        Flash_CMD_UART_Read_EnumSector(&spi_flash_memory_1, VISION_HEXADECIMAL_AND_ASCII, FLASH_SECTION_LOAD);
        	    }
        	    //FLASH_READ_ALL_SECTOR_ENUM_SECTOR_HEX_LOAD -->
        	    else if (strncmp(sub_cmd, "ENUM_SECTOR_HEX_LOAD", 20) == 0) {
        	    	char *puntero_numeros = sub_cmd + 20;
        	    	uint32_t sectorInit_temp = 0, sectorFinal_temp = 0;

        	    	if(sscanf(puntero_numeros, "%lu %lu", &sectorInit_temp, &sectorFinal_temp) == 2){
        	    		Flash_CMD_UART_Read_EnumSector_Sectors(&spi_flash_memory_1, VISION_HEXADECIMAL, FLASH_SECTION_LOAD, sectorInit_temp, sectorFinal_temp);
        	    	}

        	    }
        	    //FLASH_READ_ALL_SECTOR_ENUM_SECTOR_ASCII_LOAD -->
        	    else if (strncmp(sub_cmd, "ENUM_SECTOR_ASCII_LOAD", 22) == 0) {
        	    	char *puntero_numeros = sub_cmd + 22;
        	    	uint32_t sectorInit_temp = 0, sectorFinal_temp = 0;

        	    	if(sscanf(puntero_numeros, "%lu %lu", &sectorInit_temp, &sectorFinal_temp) == 2){
        	    		Flash_CMD_UART_Read_EnumSector_Sectors(&spi_flash_memory_1, VISION_ASCII, FLASH_SECTION_LOAD, sectorInit_temp, sectorFinal_temp);
        	    	}
        	    }
        	    //FLASH_READ_ALL_SECTOR_ENUM_SECTOR_DUAL_LOAD -->
        	    else if (strncmp(sub_cmd, "ENUM_SECTOR_DUAL_LOAD", 21) == 0) {
        	    	char *puntero_numeros = sub_cmd + 21;
					uint32_t sectorInit_temp = 0, sectorFinal_temp = 0;

					if(sscanf(puntero_numeros, "%lu %lu", &sectorInit_temp, &sectorFinal_temp) == 2){
						Flash_CMD_UART_Read_EnumSector_Sectors(&spi_flash_memory_1, VISION_HEXADECIMAL_AND_ASCII, FLASH_SECTION_LOAD, sectorInit_temp, sectorFinal_temp);
					}
        	    }
        	    else {
        	        MSG_Enviar(MSG_GENERAL,"\r\n[ERROR] SUBCMD FLASH DESCONOCIDO\r\n", Uart1);
        	    }

        	}

        	/* ==============================================================================
			 * FLASH ERASE --->
			 * ============================================================================== */
			else if (strncmp((char *)processingBuffer, "FLASH_ERASE_", 12) == 0) {

				char *sub_cmd = (char *)processingBuffer + 12;

				//FLASH_ERASE -->
				if (strncmp(sub_cmd, "ALL", 3) == 0) {
					Flash_Erase_All(&spi_flash_memory_1);
				}
				//FLASH_ERASE_PARTITION;
				else if(strncmp(sub_cmd,"PARTITION", 9) == 0)
				{
					sub_cmd = sub_cmd + 9;

					uint32_t number_partition;

					if(sscanf(sub_cmd, "%lu", &number_partition) == 1){


						if(number_partition >= FLASH_SECTION_LOAD && number_partition < FLASH_SECTION_COUNT_END) {
							Flash_Partition_ID id_partition  = (Flash_Partition_ID)number_partition;
							Flash_Erase_Partition(&spi_flash_memory_1, id_partition);

							} else {
								MSG_Enviar(MSG_GENERAL,"\r\n[ERROR] SUBCMD FLASH ERASE PARTITION DESCONOCIDA\r\n", Uart1);
							}
					}
					else
					{
						MSG_Enviar(MSG_GENERAL,"\r\n[ERROR] SUBCMD FLASH ERASE DESCONOCIDO\r\n", Uart1);
					}
				}
				else if (strncmp(sub_cmd, "ONE_", 4) == 0)
				{
					sub_cmd = sub_cmd + 4;
					//FLASH_ERASE_ONE_HEX_FOR_SECTOR
				    if (strncmp(sub_cmd, "HEX_FOR_SECTOR", 14) == 0) {
				    	char *punteros_numeros = sub_cmd + 14;
				    	uint32_t addrCommand;

				    	if (sscanf(punteros_numeros, "%lu", &addrCommand) == 1) {
				    		Flash_Erase_One_Hex_For_Sector(&spi_flash_memory_1, addrCommand);
						}
				    	else
				    	{
							MSG_Enviar(MSG_GENERAL,"\r\n[ERROR] SUBCMD FLASH ERASE DESCONOCIDO\r\n", Uart1);
						}
					}
				    //FLASH_ERASE_ONE_SECTOR_FOR_SECTOR
				    else if (strncmp(sub_cmd, "SECTOR_FOR_SECTOR", 17) == 0) {
				    	char *punteros_numeros = sub_cmd + 17;
				    	uint32_t addrCommand;

				    	if (sscanf(punteros_numeros, "%lu", &addrCommand) == 1) {
				    		Flash_Erase_One_Hex_For_Sector(&spi_flash_memory_1, addrCommand);
						}
				    	else
				    	{
							MSG_Enviar(MSG_GENERAL,"\r\n[ERROR] SUBCMD FLASH ERASE DESCONOCIDO\r\n", Uart1);
						}
					}
				    else
				    {
						MSG_Enviar(MSG_GENERAL,"\r\n[ERROR] SUBCMD FLASH ERASE DESCONOCIDO\r\n", Uart1);
					}
				}
				else if (strncmp(sub_cmd, "RANGE_", 6) == 0)
				{
					sub_cmd = sub_cmd + 6;
					//FLASH_ERASE_ONE_HEX_FOR_SECTOR
					if (strncmp(sub_cmd, "SECTOR_FOR_SECTOR", 17) == 0) {
						char *punteros_numeros = sub_cmd + 17;
						uint32_t addrInit, addrFinal;

						if (sscanf(punteros_numeros, "%lu %lu", &addrInit, &addrFinal) == 2)
						{
							Flash_Erase_Range_Hex_For_Sector(&spi_flash_memory_1, addrInit, addrFinal);
						}
						else
						{
							MSG_Enviar(MSG_GENERAL,"\r\n[ERROR] SUBCMD FLASH ERASE DESCONOCIDO\r\n", Uart1);
						}
					}
					//FLASH_ERASE_ONE_SECTR_FOR_SECTOR
					else if (strncmp(sub_cmd, "SECTOR_FOR_SECTOR", 17) == 0) {
						char *punteros_numeros = sub_cmd + 17;
						uint32_t addrCommand;

						if (sscanf(punteros_numeros, "%lu", &addrCommand) == 1) {
							Flash_Erase_One_Sector_For_Sector(&spi_flash_memory_1, addrCommand);
						}
						else
						{
							MSG_Enviar(MSG_GENERAL,"\r\n[ERROR] SUBCMD FLASH ERASE DESCONOCIDO\r\n", Uart1);
						}
					}
					else
					{
						MSG_Enviar(MSG_GENERAL,"\r\n[ERROR] SUBCMD FLASH ERASE DESCONOCIDO\r\n", Uart1);
					}
				}
				else
				{
					MSG_Enviar(MSG_GENERAL,"\r\n[ERROR] SUBCMD FLASH DESCONOCIDO\r\n", Uart1);
				}


			}
        	/* ==============================================================================
			 * TEST --->
			 * ============================================================================== */
			else if(strncmp((char *)processingBuffer, "TEST_", 5) == 0)
			{
				char *sub_cmd = (char *)processingBuffer + 5;

				/* ==============================================================================
				 * TEST FLASH --->
				 * ============================================================================== */
				if(strncmp(sub_cmd, "FLASH_", 6) == 0){

					sub_cmd = sub_cmd + 6;
					//TEST_FLASH_1
					if (strncmp(sub_cmd, "1", 1) == 0)
					{
						Test_Flash_Monitor_Inject_Valid_Data(&spi_flash_memory_1);
					}
					//TEST_FLASH_2
					else if(strncmp(sub_cmd, "2", 1) == 0){
						Test_Flash_Monitor_Inject_Valid_Data_All(&spi_flash_memory_1);
					}
					else
					{
						 MSG_Enviar(MSG_GENERAL,"COMANDO DESCONOCIDO EN TEST_FLASH", Uart1);
					}
				}
				else
				{
					 MSG_Enviar(MSG_GENERAL,"COMANDO DESCONOCIDO EN TEST_FLASH NUMBER ", Uart1);
				}
			}
			else{
        	    MSG_Enviar(MSG_GENERAL,"COMANDO DESCONOCIDO EN RX", Uart1);
        	}


    	    vTaskResume(defaultTaskHandle);

            receivedDataSize = 0;
        }
    }
}
