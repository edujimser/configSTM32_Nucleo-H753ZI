#include <string.h>
#include <stdio.h>

#include "cmsis_os.h"

#include "SPI_Flash_Memory/SPI_Flash_Memory.h"
#include "MsgUART/TxUart/TxUart.h"

extern I_TestUart TestUart_1;

bool Flash_CMD_UART_Read_All_Sectors(SPI_Flash_TypeDef *flash, E_CMD_UART_Read option)
{
    if (flash == NULL) {
        return false;
    }

    if (!flash->init || !flash->load){
    	return false;
    }

    uint8_t page_rx_buffer[FLASH_PAGE_SIZE];
    char optionFound = false;

    osDelay(TXFLASHLOG);
	   MSG_Enviar(
		   "\r\n==========================================================\r\n"
		   " [SPI FLASH] [Flash_CMD_UART_Read_All_Sectors] UARD CMD\r\n"
		   "==========================================================\r\n",
		   TestUart_1
    );
	osDelay(TXFLASHLOG);


    optionFound = (option >= 0 && option < VISION_TYPE_END);

    if(!optionFound){
    	osDelay(TXFLASHLOG);
		MSG_Enviar("\r\n [SPI FLASH] [Flash_CMD_UART_Read_All_Sectors] Opciopn erronea, opcion por defecto\r\n", TestUart_1);
		osDelay(TXFLASHLOG);

		return false;
    }

    osDelay(TXFLASHLOG);
    MSG_Enviar("\r\n [SPI FLASH] [Flash_CMD_UART_Read_All_Sectors] Iniciando volcado y visualización de datos...\r\n", TestUart_1);
    osDelay(TXFLASHLOG);




   if(option == VISION_HEXADECIMAL && optionFound){
	   for (uint32_t sector = 0; sector < FLASH_num_sector_count; ++sector) {
		  for (uint32_t page = 0; page < GET_FLASH_NUM_PAGE; ++page) {

			  uint32_t absolute_address = (sector * FLASH_SECTION_SIZE) + (page * FLASH_PAGE_SIZE);

			  uint8_t tx_unified[FLASH_PAGE_SIZE + 4];
			  uint8_t rx_unified[FLASH_PAGE_SIZE + 4];


			  memset(tx_unified, 0, FLASH_PAGE_SIZE + 4);
			  memset(rx_unified, 0, FLASH_PAGE_SIZE + 4);


			  tx_unified[0] = FLASH_CMD_READ;
			  tx_unified[1] = (uint8_t)((absolute_address >> 16) & 0xFF);
			  tx_unified[2] = (uint8_t)((absolute_address >> 8) & 0xFF);
			  tx_unified[3] = (uint8_t)(absolute_address & 0xFF);



			  if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
				  return false;
			  }


			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
			  HAL_StatusTypeDef estado_spi = HAL_SPI_TransmitReceive(flash->hspi, tx_unified, rx_unified, 260, WRITE_READ_NORMAL_VELOCITY);
			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

			  osMutexRelease(flash->spi_mutex);
			  if (estado_spi != HAL_OK) {
				  MSG_Enviar("\r\n [SPI FLASH ERROR] Fallo en TransmitReceive.\r\n", TestUart_1);
				  return false;
			  }

			  memcpy(page_rx_buffer, &rx_unified[4], FLASH_PAGE_SIZE);


	          	char log_line[LONGITUD_MAX_UART_TX];


	        	snprintf(log_line, LONGITUD_MAX_UART_TX, "\r\n=== Sector: %lu (0x%04lX) | Página: %lu (0x%04lX) ===\r\n",
	        	         (unsigned long)sector, (unsigned long)sector,
	        	         (unsigned long)page, (unsigned long)page);
	        	osDelay(TXFLASHLOG);
	        	MSG_Enviar(log_line, TestUart_1);
	        	osDelay(TXFLASHLOG);


	        	for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i += 16) {

	        	    int pos = snprintf(log_line, LONGITUD_MAX_UART_TX, "  0x%04lX: ", (unsigned long)i);

	        	    for (uint32_t j = 0; j < 16; ++j) {
	        	        if ((i + j) < FLASH_PAGE_SIZE) {
	        	            pos += snprintf(log_line + pos, LONGITUD_MAX_UART_TX - pos, "%02X ", page_rx_buffer[i + j]);
	        	        } else {
	        	            pos += snprintf(log_line + pos, LONGITUD_MAX_UART_TX - pos, "   ");
	        	        }
	        	    }

	        	    snprintf(log_line + pos, LONGITUD_MAX_UART_TX - pos, "\r\n");

	        	    osDelay(TXFLASHLOG);
	        	    MSG_Enviar(log_line, TestUart_1);
	        	    osDelay(TXFLASHLOG);
	        	}

		 }
	  }
   }

   if(option == VISION_ASCII && optionFound){
	   for (uint32_t sector = 0; sector < FLASH_num_sector_count; ++sector) {
	  		  for (uint32_t page = 0; page < GET_FLASH_NUM_PAGE; ++page) {

	  			  uint32_t absolute_address = (sector * FLASH_SECTION_SIZE) + (page * FLASH_PAGE_SIZE);

	  			  uint8_t tx_unified[260];
	  			  uint8_t rx_unified[260];


	  			  memset(tx_unified, 0, 260);
	  			  memset(rx_unified, 0, 260);


	  			  tx_unified[0] = FLASH_CMD_READ;
	  			  tx_unified[1] = (uint8_t)((absolute_address >> 16) & 0xFF);
	  			  tx_unified[2] = (uint8_t)((absolute_address >> 8) & 0xFF);
	  			  tx_unified[3] = (uint8_t)(absolute_address & 0xFF);



	  			  if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
	  				  return false;
	  			  }


	  			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	  			  HAL_StatusTypeDef estado_spi = HAL_SPI_TransmitReceive(flash->hspi, tx_unified, rx_unified, 260, WRITE_READ_NORMAL_VELOCITY);
	  			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

	  			  osMutexRelease(flash->spi_mutex);

	  			  if (estado_spi != HAL_OK) {
	  				  MSG_Enviar("\r\n [SPI FLASH ERROR] Fallo en TransmitReceive.\r\n", TestUart_1);
	  				  return false;
	  			  }

	  			  memcpy(page_rx_buffer, &rx_unified[4], 256);


	  			char log_line[LONGITUD_MAX_UART_TX];


				  snprintf(log_line, LONGITUD_MAX_UART_TX, "\r\n=== Sector: %lu (0x%04lX) | Página: %lu (0x%04lX) ===\r\n",
						   (unsigned long)sector, (unsigned long)sector,
						   (unsigned long)page, (unsigned long)page);
				  osDelay(TXFLASHLOG);
				  MSG_Enviar(log_line, TestUart_1);
				  osDelay(TXFLASHLOG);


				  for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i += 16) {

					  memset(log_line, 0, sizeof(log_line));


				      int pos = snprintf(log_line, sizeof(log_line), "  0x%04lX: ", (unsigned long)i);

				      for (uint32_t j = 0; j < 16; ++j) {
				          if ((i + j) < FLASH_PAGE_SIZE) {
				              uint8_t c = page_rx_buffer[i + j];

				              if (c >= 32 && c <= 126) {
				                  pos += snprintf(log_line + pos, sizeof(log_line) - pos, "%c", c);
				              } else {
				                  pos += snprintf(log_line + pos, sizeof(log_line) - pos, ".");
				              }
				          }
				      }

				      snprintf(log_line + pos, sizeof(log_line) - pos, "\r\n");


				      osDelay(TXFLASHLOG);
				      MSG_Enviar(log_line, TestUart_1);
				      osDelay(TXFLASHLOG);
				  }


	  		 }
	  	}
   }

   if(option == VISION_HEXADECIMAL_AND_ASCII && optionFound){
	   for (uint32_t sector = 0; sector < FLASH_num_sector_count; ++sector) {
		  for (uint32_t page = 0; page < GET_FLASH_NUM_PAGE; ++page) {

			  uint32_t absolute_address = (sector * FLASH_SECTION_SIZE) + (page * FLASH_PAGE_SIZE);

			  uint8_t tx_unified[260];
			  uint8_t rx_unified[260];


			  memset(tx_unified, 0, 260);
			  memset(rx_unified, 0, 260);


			  tx_unified[0] = FLASH_CMD_READ;
			  tx_unified[1] = (uint8_t)((absolute_address >> 16) & 0xFF);
			  tx_unified[2] = (uint8_t)((absolute_address >> 8) & 0xFF);
			  tx_unified[3] = (uint8_t)(absolute_address & 0xFF);



			  if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
				  return false;
			  }


			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
			  HAL_StatusTypeDef estado_spi = HAL_SPI_TransmitReceive(flash->hspi, tx_unified, rx_unified, 260, WRITE_READ_NORMAL_VELOCITY);
			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

			  osMutexRelease(flash->spi_mutex);

			  if (estado_spi != HAL_OK) {
				  MSG_Enviar("\r\n [SPI FLASH ERROR] Fallo en TransmitReceive.\r\n", TestUart_1);
				  return false;
			  }

			  memcpy(page_rx_buffer, &rx_unified[4], 256);


          	char log_line[LONGITUD_MAX_UART_TX];


			  snprintf(log_line, LONGITUD_MAX_UART_TX, "\r\n=== Sector: %lu (0x%04lX) | Página: %lu (0x%04lX) ===\r\n",
					   (unsigned long)sector, (unsigned long)sector,
					   (unsigned long)page, (unsigned long)page);
			  osDelay(TXFLASHLOG);
			  MSG_Enviar(log_line, TestUart_1);
			  osDelay(TXFLASHLOG);


			  for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i += 16) {

			  				  memset(log_line, 0, sizeof(log_line));

			      int pos = snprintf(log_line, sizeof(log_line), "  0x%04lX: ", (unsigned long)i);


			      for (uint32_t j = 0; j < 16; ++j) {
			          if ((i + j) < FLASH_PAGE_SIZE) {
			              pos += snprintf(log_line + pos, sizeof(log_line) - pos, "%02X ", page_rx_buffer[i + j]);
			          } else {
			              pos += snprintf(log_line + pos, sizeof(log_line) - pos, "   ");
			          }
			      }


			      pos += snprintf(log_line + pos, sizeof(log_line) - pos, " | ");


			      for (uint32_t j = 0; j < 16; ++j) {
			          if ((i + j) < FLASH_PAGE_SIZE) {
			              uint8_t c = page_rx_buffer[i + j];

			              if (c >= 32 && c <= 126) {
			                  pos += snprintf(log_line + pos, sizeof(log_line) - pos, "%c", c);
			              } else {
			                  pos += snprintf(log_line + pos, sizeof(log_line) - pos, ".");
			              }
			          }
			      }


			      snprintf(log_line + pos, sizeof(log_line) - pos, "\r\n");


			      osDelay(TXFLASHLOG);
			      MSG_Enviar(log_line, TestUart_1);
			      osDelay(TXFLASHLOG);
			  }
		  }
	  }
   }




    osDelay(TXFLASHLOG);
	   MSG_Enviar(
		   "\r\n==========================================================\r\n"
		   " [SPI FLASH] [Flash_CMD_UART_Read_All_Sectors] UARD CMD - FIN \r\n"
		   "==========================================================\r\n",
		   TestUart_1
    );
	osDelay(TXFLASHLOG);


	Flash_Complement_Data_Thread(FLASH_DEBUG_THREAD_SIZE);

    return true;
}

bool Flash_CMD_UART_Read_Range_Sectors_Page(SPI_Flash_TypeDef *flash, E_CMD_UART_Read option, uint32_t sectorInit, uint32_t sectorFinal, uint16_t numPage)
{
    if (flash == NULL) {
        return false;
    }

    if (!flash->init || !flash->load){
    	return false;
    }

    uint8_t page_rx_buffer[FLASH_PAGE_SIZE];
    char optionFound = false;

    osDelay(TXFLASHLOG);
	   MSG_Enviar(
		   "\r\n==========================================================\r\n"
		   " [SPI FLASH] [Flash_CMD_UART_Read_Range_Sectors_Page] UARD CMD\r\n"
		   "==========================================================\r\n",
		   TestUart_1
    );
	osDelay(TXFLASHLOG);


    optionFound = (option >= 0 && option < VISION_TYPE_END);

    if(!optionFound){
    	osDelay(TXFLASHLOG);
		MSG_Enviar("\r\n [SPI FLASH] [Flash_CMD_UART_Read_Range_Sectors_Page] Opciopn erronea, opcion por defecto\r\n", TestUart_1);
		osDelay(TXFLASHLOG);

		return false;
    }

    if(sectorInit < 0)sectorInit = 0;
    if(sectorFinal > GET_FLASH_CAPCITY)sectorFinal = GET_FLASH_CAPCITY;
    if(numPage > GET_FLASH_NUM_PAGE) numPage = GET_FLASH_NUM_PAGE;
    if(sectorFinal == 0 || numPage == 0) return false;

    osDelay(TXFLASHLOG);
    MSG_Enviar("\r\n [SPI FLASH] [Flash_CMD_UART_Read_Range_Sectors_Page] Iniciando volcado y visualización de datos...\r\n", TestUart_1);
    osDelay(TXFLASHLOG);




   if(option == VISION_HEXADECIMAL && optionFound){
	   for (uint32_t sector = 0; sector < sectorFinal; ++sector) {
		  for (uint32_t page = 0; page < numPage; ++page) {

			  uint32_t absolute_address = (sector * FLASH_SECTION_SIZE) + (page * FLASH_PAGE_SIZE);

			  uint8_t tx_unified[FLASH_PAGE_SIZE + 4];
			  uint8_t rx_unified[FLASH_PAGE_SIZE + 4];


			  memset(tx_unified, 0, FLASH_PAGE_SIZE + 4);
			  memset(rx_unified, 0, FLASH_PAGE_SIZE + 4);


			  tx_unified[0] = FLASH_CMD_READ;
			  tx_unified[1] = (uint8_t)((absolute_address >> 16) & 0xFF);
			  tx_unified[2] = (uint8_t)((absolute_address >> 8) & 0xFF);
			  tx_unified[3] = (uint8_t)(absolute_address & 0xFF);



			  if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
				  return false;
			  }


			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
			  HAL_StatusTypeDef estado_spi = HAL_SPI_TransmitReceive(flash->hspi, tx_unified, rx_unified, 260, WRITE_READ_NORMAL_VELOCITY);
			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

			  osMutexRelease(flash->spi_mutex);
			  if (estado_spi != HAL_OK) {
				  MSG_Enviar("\r\n [SPI FLASH ERROR] Fallo en TransmitReceive.\r\n", TestUart_1);
				  return false;
			  }

			  memcpy(page_rx_buffer, &rx_unified[4], FLASH_PAGE_SIZE);


	          	char log_line[LONGITUD_MAX_UART_TX];


	        	snprintf(log_line, LONGITUD_MAX_UART_TX, "\r\n=== Sector: %lu (0x%04lX) | Página: %lu (0x%04lX) ===\r\n",
	        	         (unsigned long)sector, (unsigned long)sector,
	        	         (unsigned long)page, (unsigned long)page);
	        	osDelay(TXFLASHLOG);
	        	MSG_Enviar(log_line, TestUart_1);
	        	osDelay(TXFLASHLOG);


	        	for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i += 16) {

	        	    int pos = snprintf(log_line, LONGITUD_MAX_UART_TX, "  0x%04lX: ", (unsigned long)i);

	        	    for (uint32_t j = 0; j < 16; ++j) {
	        	        if ((i + j) < FLASH_PAGE_SIZE) {
	        	            pos += snprintf(log_line + pos, LONGITUD_MAX_UART_TX - pos, "%02X ", page_rx_buffer[i + j]);
	        	        } else {
	        	            pos += snprintf(log_line + pos, LONGITUD_MAX_UART_TX - pos, "   ");
	        	        }
	        	    }

	        	    snprintf(log_line + pos, LONGITUD_MAX_UART_TX - pos, "\r\n");

	        	    osDelay(TXFLASHLOG);
	        	    MSG_Enviar(log_line, TestUart_1);
	        	    osDelay(TXFLASHLOG);
	        	}

		 }
	  }
   }

   if(option == VISION_ASCII && optionFound){
	   for (uint32_t sector = 0; sector < sectorFinal; ++sector) {
		  for (uint32_t page = 0; page < numPage; ++page) {

	  			  uint32_t absolute_address = (sector * FLASH_SECTION_SIZE) + (page * FLASH_PAGE_SIZE);

	  			  uint8_t tx_unified[260];
	  			  uint8_t rx_unified[260];


	  			  memset(tx_unified, 0, 260);
	  			  memset(rx_unified, 0, 260);


	  			  tx_unified[0] = FLASH_CMD_READ;
	  			  tx_unified[1] = (uint8_t)((absolute_address >> 16) & 0xFF);
	  			  tx_unified[2] = (uint8_t)((absolute_address >> 8) & 0xFF);
	  			  tx_unified[3] = (uint8_t)(absolute_address & 0xFF);



	  			  if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
	  				  return false;
	  			  }


	  			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	  			  HAL_StatusTypeDef estado_spi = HAL_SPI_TransmitReceive(flash->hspi, tx_unified, rx_unified, 260, WRITE_READ_NORMAL_VELOCITY);
	  			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

	  			  osMutexRelease(flash->spi_mutex);

	  			  if (estado_spi != HAL_OK) {
	  				  MSG_Enviar("\r\n [SPI FLASH ERROR] Fallo en TransmitReceive.\r\n", TestUart_1);
	  				  return false;
	  			  }

	  			  memcpy(page_rx_buffer, &rx_unified[4], 256);


	  			char log_line[LONGITUD_MAX_UART_TX];


				  snprintf(log_line, LONGITUD_MAX_UART_TX, "\r\n=== Sector: %lu (0x%04lX) | Página: %lu (0x%04lX) ===\r\n",
						   (unsigned long)sector, (unsigned long)sector,
						   (unsigned long)page, (unsigned long)page);
				  osDelay(TXFLASHLOG);
				  MSG_Enviar(log_line, TestUart_1);
				  osDelay(TXFLASHLOG);


				  for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i += 16) {

					  memset(log_line, 0, sizeof(log_line));


				      int pos = snprintf(log_line, sizeof(log_line), "  0x%04lX: ", (unsigned long)i);

				      for (uint32_t j = 0; j < 16; ++j) {
				          if ((i + j) < FLASH_PAGE_SIZE) {
				              uint8_t c = page_rx_buffer[i + j];

				              if (c >= 32 && c <= 126) {
				                  pos += snprintf(log_line + pos, sizeof(log_line) - pos, "%c", c);
				              } else {
				                  pos += snprintf(log_line + pos, sizeof(log_line) - pos, ".");
				              }
				          }
				      }

				      snprintf(log_line + pos, sizeof(log_line) - pos, "\r\n");


				      osDelay(TXFLASHLOG);
				      MSG_Enviar(log_line, TestUart_1);
				      osDelay(TXFLASHLOG);
				  }


	  		 }
	  	}
   }

   if(option == VISION_HEXADECIMAL_AND_ASCII && optionFound){
	   for (uint32_t sector = 0; sector < sectorFinal; ++sector) {
		  for (uint32_t page = 0; page < numPage; ++page) {

			  uint32_t absolute_address = (sector * FLASH_SECTION_SIZE) + (page * FLASH_PAGE_SIZE);

			  uint8_t tx_unified[260];
			  uint8_t rx_unified[260];


			  memset(tx_unified, 0, 260);
			  memset(rx_unified, 0, 260);


			  tx_unified[0] = FLASH_CMD_READ;
			  tx_unified[1] = (uint8_t)((absolute_address >> 16) & 0xFF);
			  tx_unified[2] = (uint8_t)((absolute_address >> 8) & 0xFF);
			  tx_unified[3] = (uint8_t)(absolute_address & 0xFF);



			  if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
				  return false;
			  }


			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
			  HAL_StatusTypeDef estado_spi = HAL_SPI_TransmitReceive(flash->hspi, tx_unified, rx_unified, 260, WRITE_READ_NORMAL_VELOCITY);
			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

			  osMutexRelease(flash->spi_mutex);

			  if (estado_spi != HAL_OK) {
				  MSG_Enviar("\r\n [SPI FLASH ERROR] Fallo en TransmitReceive.\r\n", TestUart_1);
				  return false;
			  }

			  memcpy(page_rx_buffer, &rx_unified[4], 256);


          	char log_line[LONGITUD_MAX_UART_TX];


			  snprintf(log_line, LONGITUD_MAX_UART_TX, "\r\n=== Sector: %lu (0x%04lX) | Página: %lu (0x%04lX) ===\r\n",
					   (unsigned long)sector, (unsigned long)sector,
					   (unsigned long)page, (unsigned long)page);
			  osDelay(TXFLASHLOG);
			  MSG_Enviar(log_line, TestUart_1);
			  osDelay(TXFLASHLOG);


			  for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i += 16) {

			  				  memset(log_line, 0, sizeof(log_line));

			      int pos = snprintf(log_line, sizeof(log_line), "  0x%04lX: ", (unsigned long)i);


			      for (uint32_t j = 0; j < 16; ++j) {
			          if ((i + j) < FLASH_PAGE_SIZE) {
			              pos += snprintf(log_line + pos, sizeof(log_line) - pos, "%02X ", page_rx_buffer[i + j]);
			          } else {
			              pos += snprintf(log_line + pos, sizeof(log_line) - pos, "   ");
			          }
			      }


			      pos += snprintf(log_line + pos, sizeof(log_line) - pos, " | ");


			      for (uint32_t j = 0; j < 16; ++j) {
			          if ((i + j) < FLASH_PAGE_SIZE) {
			              uint8_t c = page_rx_buffer[i + j];

			              if (c >= 32 && c <= 126) {
			                  pos += snprintf(log_line + pos, sizeof(log_line) - pos, "%c", c);
			              } else {
			                  pos += snprintf(log_line + pos, sizeof(log_line) - pos, ".");
			              }
			          }
			      }


			      snprintf(log_line + pos, sizeof(log_line) - pos, "\r\n");


			      osDelay(TXFLASHLOG);
			      MSG_Enviar(log_line, TestUart_1);
			      osDelay(TXFLASHLOG);
			  }
		  }
	  }
   }




    osDelay(TXFLASHLOG);
	   MSG_Enviar(
		   "\r\n==========================================================\r\n"
		   " [SPI FLASH] [Flash_CMD_UART_Read_All_Sectors] UARD CMD - FIN \r\n"
		   "==========================================================\r\n",
		   TestUart_1
    );
	osDelay(TXFLASHLOG);


	Flash_Complement_Data_Thread(FLASH_DEBUG_THREAD_SIZE);

    return true;
}

bool Flash_CMD_UART_Read_EnumSector(SPI_Flash_TypeDef *flash, E_CMD_UART_Read option, uint32_t ID)
{
    if (flash == NULL) {
        return false;
    }

    if (!flash->init || !flash->load){
    	return false;
    }

    uint8_t page_rx_buffer[FLASH_PAGE_SIZE];
    char optionFound = false;
    bool optionFoundID = false;

    osDelay(TXFLASHLOG);
	   MSG_Enviar(
		   "\r\n==========================================================\r\n"
		   " [SPI FLASH] [Flash_CMD_UART_Read_EnumSector] UARD CMD\r\n"
		   "==========================================================\r\n",
		   TestUart_1
    );
	osDelay(TXFLASHLOG);


    optionFound = (option >= 0 && option < VISION_TYPE_END);
    optionFoundID = (ID >= 0 && ID < FLASH_SECTION_COUNT_END);

    if(!optionFound){
    	osDelay(TXFLASHLOG);
		MSG_Enviar("\r\n [SPI FLASH] [Flash_CMD_UART_Read_EnumSector] Opciopn erronea, opcion por defecto\r\n", TestUart_1);
		osDelay(TXFLASHLOG);

		return false;
    }


    if(!optionFoundID){
    	osDelay(TXFLASHLOG);
		MSG_Enviar("\r\n [SPI FLASH] [Flash_CMD_UART_Read_EnumSector] Sector errone o no existente\r\n", TestUart_1);
		osDelay(TXFLASHLOG);

		return false;
    }

    osDelay(TXFLASHLOG);
    MSG_Enviar("\r\n [SPI FLASH] [Flash_CMD_UART_Read_EnumSector] Iniciando volcado y visualización de datos...\r\n", TestUart_1);
    osDelay(TXFLASHLOG);


   uint64_t sectorInitID = flash->partitions[ID].num_start_sector;
   uint64_t sectorFinalID =	flash->partitions[ID].num_end_sector;

   if(option == VISION_HEXADECIMAL && optionFound && optionFoundID){
	   for (uint32_t sector = sectorInitID; sector <= sectorFinalID; sector = sector + FLASH_SECTION_SIZE) {
		  for (uint32_t page = 0; page < GET_FLASH_NUM_PAGE; ++page) {

			  uint32_t absolute_address = (sector) + (page * FLASH_PAGE_SIZE);

			  uint8_t tx_unified[FLASH_PAGE_SIZE + 4];
			  uint8_t rx_unified[FLASH_PAGE_SIZE + 4];


			  memset(tx_unified, 0, FLASH_PAGE_SIZE + 4);
			  memset(rx_unified, 0, FLASH_PAGE_SIZE + 4);


			  tx_unified[0] = FLASH_CMD_READ;
			  tx_unified[1] = (uint8_t)((absolute_address >> 16) & 0xFF);
			  tx_unified[2] = (uint8_t)((absolute_address >> 8) & 0xFF);
			  tx_unified[3] = (uint8_t)(absolute_address & 0xFF);



			  if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
				  return false;
			  }


			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
			  HAL_StatusTypeDef estado_spi = HAL_SPI_TransmitReceive(flash->hspi, tx_unified, rx_unified, 260, WRITE_READ_NORMAL_VELOCITY);
			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

			  osMutexRelease(flash->spi_mutex);
			  if (estado_spi != HAL_OK) {
				  MSG_Enviar("\r\n [SPI FLASH ERROR] Fallo en TransmitReceive.\r\n", TestUart_1);
				  return false;
			  }

			  memcpy(page_rx_buffer, &rx_unified[4], FLASH_PAGE_SIZE);


	          	char log_line[LONGITUD_MAX_UART_TX];


	        	snprintf(log_line, LONGITUD_MAX_UART_TX, "\r\n=== Sector: %lu (0x%04lX) | Página: %lu (0x%04lX) ===\r\n",
	        	         (unsigned long)sector, (unsigned long)sector,
	        	         (unsigned long)page, (unsigned long)page);
	        	osDelay(TXFLASHLOG);
	        	MSG_Enviar(log_line, TestUart_1);
	        	osDelay(TXFLASHLOG);


	        	for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i += 16) {

	        	    int pos = snprintf(log_line, LONGITUD_MAX_UART_TX, "  0x%04lX: ", (unsigned long)i);

	        	    for (uint32_t j = 0; j < 16; ++j) {
	        	        if ((i + j) < FLASH_PAGE_SIZE) {
	        	            pos += snprintf(log_line + pos, LONGITUD_MAX_UART_TX - pos, "%02X ", page_rx_buffer[i + j]);
	        	        } else {
	        	            pos += snprintf(log_line + pos, LONGITUD_MAX_UART_TX - pos, "   ");
	        	        }
	        	    }

	        	    snprintf(log_line + pos, LONGITUD_MAX_UART_TX - pos, "\r\n");

	        	    osDelay(TXFLASHLOG);
	        	    MSG_Enviar(log_line, TestUart_1);
	        	    osDelay(TXFLASHLOG);
	        	}

		 }
	  }
   }

   if(option == VISION_ASCII && optionFound && optionFoundID){
	   for (uint32_t sector = sectorInitID; sector <= sectorFinalID; ++sector) {
		  for (uint32_t page = 0; page < GET_FLASH_NUM_PAGE; ++page) {

	  			  uint32_t absolute_address = (sector * FLASH_SECTION_SIZE) + (page * FLASH_PAGE_SIZE);

	  			  uint8_t tx_unified[260];
	  			  uint8_t rx_unified[260];


	  			  memset(tx_unified, 0, 260);
	  			  memset(rx_unified, 0, 260);


	  			  tx_unified[0] = FLASH_CMD_READ;
	  			  tx_unified[1] = (uint8_t)((absolute_address >> 16) & 0xFF);
	  			  tx_unified[2] = (uint8_t)((absolute_address >> 8) & 0xFF);
	  			  tx_unified[3] = (uint8_t)(absolute_address & 0xFF);



	  			  if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
	  				  return false;
	  			  }


	  			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	  			  HAL_StatusTypeDef estado_spi = HAL_SPI_TransmitReceive(flash->hspi, tx_unified, rx_unified, 260, WRITE_READ_NORMAL_VELOCITY);
	  			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

	  			  osMutexRelease(flash->spi_mutex);

	  			  if (estado_spi != HAL_OK) {
	  				  MSG_Enviar("\r\n [SPI FLASH ERROR] Fallo en TransmitReceive.\r\n", TestUart_1);
	  				  return false;
	  			  }

	  			  memcpy(page_rx_buffer, &rx_unified[4], 256);


	  			char log_line[LONGITUD_MAX_UART_TX];


				  snprintf(log_line, LONGITUD_MAX_UART_TX, "\r\n=== Sector: %lu (0x%04lX) | Página: %lu (0x%04lX) ===\r\n",
						   (unsigned long)sector, (unsigned long)sector,
						   (unsigned long)page, (unsigned long)page);
				  osDelay(TXFLASHLOG);
				  MSG_Enviar(log_line, TestUart_1);
				  osDelay(TXFLASHLOG);


				  for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i += 16) {

					  memset(log_line, 0, sizeof(log_line));


				      int pos = snprintf(log_line, sizeof(log_line), "  0x%04lX: ", (unsigned long)i);

				      for (uint32_t j = 0; j < 16; ++j) {
				          if ((i + j) < FLASH_PAGE_SIZE) {
				              uint8_t c = page_rx_buffer[i + j];

				              if (c >= 32 && c <= 126) {
				                  pos += snprintf(log_line + pos, sizeof(log_line) - pos, "%c", c);
				              } else {
				                  pos += snprintf(log_line + pos, sizeof(log_line) - pos, ".");
				              }
				          }
				      }

				      snprintf(log_line + pos, sizeof(log_line) - pos, "\r\n");


				      osDelay(TXFLASHLOG);
				      MSG_Enviar(log_line, TestUart_1);
				      osDelay(TXFLASHLOG);
				  }


	  		 }
	  	}
   }

   if(option == VISION_HEXADECIMAL_AND_ASCII && optionFound && optionFoundID){
	   for (uint32_t sector = sectorInitID; sector <= sectorFinalID; ++sector) {
		  for (uint32_t page = 0; page < GET_FLASH_NUM_PAGE; ++page) {

			  uint32_t absolute_address = (sector * FLASH_SECTION_SIZE) + (page * FLASH_PAGE_SIZE);

			  uint8_t tx_unified[260];
			  uint8_t rx_unified[260];


			  memset(tx_unified, 0, 260);
			  memset(rx_unified, 0, 260);


			  tx_unified[0] = FLASH_CMD_READ;
			  tx_unified[1] = (uint8_t)((absolute_address >> 16) & 0xFF);
			  tx_unified[2] = (uint8_t)((absolute_address >> 8) & 0xFF);
			  tx_unified[3] = (uint8_t)(absolute_address & 0xFF);



			  if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
				  return false;
			  }


			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
			  HAL_StatusTypeDef estado_spi = HAL_SPI_TransmitReceive(flash->hspi, tx_unified, rx_unified, 260, WRITE_READ_NORMAL_VELOCITY);
			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

			  osMutexRelease(flash->spi_mutex);

			  if (estado_spi != HAL_OK) {
				  MSG_Enviar("\r\n [SPI FLASH ERROR] Fallo en TransmitReceive.\r\n", TestUart_1);
				  return false;
			  }

			  memcpy(page_rx_buffer, &rx_unified[4], 256);


          	char log_line[LONGITUD_MAX_UART_TX];


			  snprintf(log_line, LONGITUD_MAX_UART_TX, "\r\n=== Sector: %lu (0x%04lX) | Página: %lu (0x%04lX) ===\r\n",
					   (unsigned long)sector, (unsigned long)sector,
					   (unsigned long)page, (unsigned long)page);
			  osDelay(TXFLASHLOG);
			  MSG_Enviar(log_line, TestUart_1);
			  osDelay(TXFLASHLOG);


			  for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i += 16) {

			  				  memset(log_line, 0, sizeof(log_line));

			      int pos = snprintf(log_line, sizeof(log_line), "  0x%04lX: ", (unsigned long)i);


			      for (uint32_t j = 0; j < 16; ++j) {
			          if ((i + j) < FLASH_PAGE_SIZE) {
			              pos += snprintf(log_line + pos, sizeof(log_line) - pos, "%02X ", page_rx_buffer[i + j]);
			          } else {
			              pos += snprintf(log_line + pos, sizeof(log_line) - pos, "   ");
			          }
			      }


			      pos += snprintf(log_line + pos, sizeof(log_line) - pos, " | ");


			      for (uint32_t j = 0; j < 16; ++j) {
			          if ((i + j) < FLASH_PAGE_SIZE) {
			              uint8_t c = page_rx_buffer[i + j];

			              if (c >= 32 && c <= 126) {
			                  pos += snprintf(log_line + pos, sizeof(log_line) - pos, "%c", c);
			              } else {
			                  pos += snprintf(log_line + pos, sizeof(log_line) - pos, ".");
			              }
			          }
			      }


			      snprintf(log_line + pos, sizeof(log_line) - pos, "\r\n");


			      osDelay(TXFLASHLOG);
			      MSG_Enviar(log_line, TestUart_1);
			      osDelay(TXFLASHLOG);
			  }
		  }
	  }
   }




    osDelay(TXFLASHLOG);
	   MSG_Enviar(
		   "\r\n==========================================================\r\n"
		   " [SPI FLASH] [Flash_CMD_UART_Read_All_Sectors] UARD CMD - FIN \r\n"
		   "==========================================================\r\n",
		   TestUart_1
    );
	osDelay(TXFLASHLOG);


	Flash_Complement_Data_Thread(FLASH_DEBUG_THREAD_SIZE);

    return true;
}

bool Flash_CMD_UART_Read_EnumSector_Sectors(SPI_Flash_TypeDef *flash, E_CMD_UART_Read option, uint32_t ID, uint32_t sectorInit, uint32_t sectorFinal)
{
    if (flash == NULL) {
        return false;
    }

    if (!flash->init || !flash->load){
    	return false;
    }

    uint8_t page_rx_buffer[FLASH_PAGE_SIZE];
    char optionFound = false;
    bool optionFoundID = false;
    bool protectedLimit = false;

    osDelay(TXFLASHLOG);
	   MSG_Enviar(
		   "\r\n==========================================================\r\n"
		   " [SPI FLASH] [Flash_CMD_UART_Read_All_Sectors] UARD CMD\r\n"
		   "==========================================================\r\n",
		   TestUart_1
    );
	osDelay(TXFLASHLOG);

	uint32_t addrInit = sectorInit * FLASH_SECTION_SIZE;
    uint32_t addrEnd = sectorFinal * FLASH_SECTION_SIZE;


    optionFound = (option >= 0 && option < VISION_TYPE_END);
    optionFoundID = (ID >= 0 && ID < FLASH_SECTION_COUNT_END);
    protectedLimit = Flash_Complement_protetionLimit_Range(flash, addrInit, addrEnd);


    if(!optionFound){
    	osDelay(TXFLASHLOG);
		MSG_Enviar("\r\n [SPI FLASH] [Flash_CMD_UART_Read_EnumSector_Sectors] Opciopn erronea, opcion por defecto\r\n", TestUart_1);
		osDelay(TXFLASHLOG);

		return false;
    }


    if(!optionFoundID){
    	osDelay(TXFLASHLOG);
		MSG_Enviar("\r\n [SPI FLASH] [Flash_CMD_UART_Read_EnumSector_Sectors] Sector final incorrecto o fuera de rango\r\n", TestUart_1);
		osDelay(TXFLASHLOG);

		return false;
    }


    if(!protectedLimit){
    	osDelay(TXFLASHLOG);
		MSG_Enviar("\r\n [SPI FLASH] [Flash_CMD_UART_Read_EnumSector_Sectors] Fuera de rango uera de rango\r\n", TestUart_1);
		osDelay(TXFLASHLOG);

		return false;
    }

    osDelay(TXFLASHLOG);
    MSG_Enviar("\r\n [SPI FLASH] [Flash_CMD_UART_Read_EnumSector_Sectors] Iniciando volcado y visualización de datos...\r\n", TestUart_1);
    osDelay(TXFLASHLOG);


   if(option == VISION_HEXADECIMAL && optionFound && optionFoundID){
	   for (uint32_t sector = addrInit; sector <= addrEnd; sector = sector +FLASH_SECTION_SIZE) {
		  for (uint32_t page = 0; page < GET_FLASH_NUM_PAGE; ++page) {

			  uint32_t absolute_address = sector  + (page * FLASH_PAGE_SIZE);

			  uint8_t tx_unified[FLASH_PAGE_SIZE + 4];
			  uint8_t rx_unified[FLASH_PAGE_SIZE + 4];


			  memset(tx_unified, 0, FLASH_PAGE_SIZE + 4);
			  memset(rx_unified, 0, FLASH_PAGE_SIZE + 4);


			  tx_unified[0] = FLASH_CMD_READ;
			  tx_unified[1] = (uint8_t)((absolute_address >> 16) & 0xFF);
			  tx_unified[2] = (uint8_t)((absolute_address >> 8) & 0xFF);
			  tx_unified[3] = (uint8_t)(absolute_address & 0xFF);



			  if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
				  return false;
			  }


			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
			  HAL_StatusTypeDef estado_spi = HAL_SPI_TransmitReceive(flash->hspi, tx_unified, rx_unified, 260, WRITE_READ_NORMAL_VELOCITY);
			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

			  osMutexRelease(flash->spi_mutex);
			  if (estado_spi != HAL_OK) {
				  MSG_Enviar("\r\n [SPI FLASH ERROR] Fallo en TransmitReceive.\r\n", TestUart_1);
				  return false;
			  }

			  memcpy(page_rx_buffer, &rx_unified[4], FLASH_PAGE_SIZE);


	          	char log_line[LONGITUD_MAX_UART_TX];


	        	snprintf(log_line, LONGITUD_MAX_UART_TX, "\r\n=== Sector: %lu (0x%04lX) | Página: %lu (0x%04lX) ===\r\n",
	        	         (unsigned long)sector, (unsigned long)sector,
	        	         (unsigned long)page, (unsigned long)page);
	        	osDelay(TXFLASHLOG);
	        	MSG_Enviar(log_line, TestUart_1);
	        	osDelay(TXFLASHLOG);


	        	for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i += 16) {

	        	    int pos = snprintf(log_line, LONGITUD_MAX_UART_TX, "  0x%04lX: ", (unsigned long)i);

	        	    for (uint32_t j = 0; j < 16; ++j) {
	        	        if ((i + j) < FLASH_PAGE_SIZE) {
	        	            pos += snprintf(log_line + pos, LONGITUD_MAX_UART_TX - pos, "%02X ", page_rx_buffer[i + j]);
	        	        } else {
	        	            pos += snprintf(log_line + pos, LONGITUD_MAX_UART_TX - pos, "   ");
	        	        }
	        	    }

	        	    snprintf(log_line + pos, LONGITUD_MAX_UART_TX - pos, "\r\n");

	        	    osDelay(TXFLASHLOG);
	        	    MSG_Enviar(log_line, TestUart_1);
	        	    osDelay(TXFLASHLOG);
	        	}

		 }
	  }
   }

   if(option == VISION_ASCII && optionFound && optionFoundID){
	   for (uint32_t sector = addrInit; sector <= addrEnd; sector = sector +FLASH_SECTION_SIZE) {
	  		  for (uint32_t page = 0; page < GET_FLASH_NUM_PAGE; ++page) {

	  			  uint32_t absolute_address = sector  + (page * FLASH_PAGE_SIZE);

	  			  uint8_t tx_unified[260];
	  			  uint8_t rx_unified[260];


	  			  memset(tx_unified, 0, 260);
	  			  memset(rx_unified, 0, 260);


	  			  tx_unified[0] = FLASH_CMD_READ;
	  			  tx_unified[1] = (uint8_t)((absolute_address >> 16) & 0xFF);
	  			  tx_unified[2] = (uint8_t)((absolute_address >> 8) & 0xFF);
	  			  tx_unified[3] = (uint8_t)(absolute_address & 0xFF);



	  			  if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
	  				  return false;
	  			  }


	  			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	  			  HAL_StatusTypeDef estado_spi = HAL_SPI_TransmitReceive(flash->hspi, tx_unified, rx_unified, 260, WRITE_READ_NORMAL_VELOCITY);
	  			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

	  			  osMutexRelease(flash->spi_mutex);

	  			  if (estado_spi != HAL_OK) {
	  				  MSG_Enviar("\r\n [SPI FLASH ERROR] Fallo en TransmitReceive.\r\n", TestUart_1);
	  				  return false;
	  			  }

	  			  memcpy(page_rx_buffer, &rx_unified[4], 256);


	  			char log_line[LONGITUD_MAX_UART_TX];


				  snprintf(log_line, LONGITUD_MAX_UART_TX, "\r\n=== Sector: %lu (0x%04lX) | Página: %lu (0x%04lX) ===\r\n",
						   (unsigned long)sector, (unsigned long)sector,
						   (unsigned long)page, (unsigned long)page);
				  osDelay(TXFLASHLOG);
				  MSG_Enviar(log_line, TestUart_1);
				  osDelay(TXFLASHLOG);


				  for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i += 16) {

					  memset(log_line, 0, sizeof(log_line));


				      int pos = snprintf(log_line, sizeof(log_line), "  0x%04lX: ", (unsigned long)i);

				      for (uint32_t j = 0; j < 16; ++j) {
				          if ((i + j) < FLASH_PAGE_SIZE) {
				              uint8_t c = page_rx_buffer[i + j];

				              if (c >= 32 && c <= 126) {
				                  pos += snprintf(log_line + pos, sizeof(log_line) - pos, "%c", c);
				              } else {
				                  pos += snprintf(log_line + pos, sizeof(log_line) - pos, ".");
				              }
				          }
				      }

				      snprintf(log_line + pos, sizeof(log_line) - pos, "\r\n");


				      osDelay(TXFLASHLOG);
				      MSG_Enviar(log_line, TestUart_1);
				      osDelay(TXFLASHLOG);
				  }


	  		 }
	  	}
   }

   if(option == VISION_HEXADECIMAL_AND_ASCII && optionFound && optionFoundID){
	   for (uint32_t sector = addrInit; sector <= addrEnd; sector = sector +FLASH_SECTION_SIZE) {
		  for (uint32_t page = 0; page < GET_FLASH_NUM_PAGE; ++page) {

			  uint32_t absolute_address = sector  + (page * FLASH_PAGE_SIZE);

			  uint8_t tx_unified[260];
			  uint8_t rx_unified[260];


			  memset(tx_unified, 0, 260);
			  memset(rx_unified, 0, 260);


			  tx_unified[0] = FLASH_CMD_READ;
			  tx_unified[1] = (uint8_t)((absolute_address >> 16) & 0xFF);
			  tx_unified[2] = (uint8_t)((absolute_address >> 8) & 0xFF);
			  tx_unified[3] = (uint8_t)(absolute_address & 0xFF);



			  if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
				  return false;
			  }


			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
			  HAL_StatusTypeDef estado_spi = HAL_SPI_TransmitReceive(flash->hspi, tx_unified, rx_unified, 260, WRITE_READ_NORMAL_VELOCITY);
			  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

			  osMutexRelease(flash->spi_mutex);

			  if (estado_spi != HAL_OK) {
				  MSG_Enviar("\r\n [SPI FLASH ERROR] Fallo en TransmitReceive.\r\n", TestUart_1);
				  return false;
			  }

			  memcpy(page_rx_buffer, &rx_unified[4], 256);


          	char log_line[LONGITUD_MAX_UART_TX];


			  snprintf(log_line, LONGITUD_MAX_UART_TX, "\r\n=== Sector: %lu (0x%04lX) | Página: %lu (0x%04lX) ===\r\n",
					   (unsigned long)sector, (unsigned long)sector,
					   (unsigned long)page, (unsigned long)page);
			  osDelay(TXFLASHLOG);
			  MSG_Enviar(log_line, TestUart_1);
			  osDelay(TXFLASHLOG);


			  for (uint32_t i = 0; i < FLASH_PAGE_SIZE; i += 16) {

			  				  memset(log_line, 0, sizeof(log_line));

			      int pos = snprintf(log_line, sizeof(log_line), "  0x%04lX: ", (unsigned long)i);


			      for (uint32_t j = 0; j < 16; ++j) {
			          if ((i + j) < FLASH_PAGE_SIZE) {
			              pos += snprintf(log_line + pos, sizeof(log_line) - pos, "%02X ", page_rx_buffer[i + j]);
			          } else {
			              pos += snprintf(log_line + pos, sizeof(log_line) - pos, "   ");
			          }
			      }


			      pos += snprintf(log_line + pos, sizeof(log_line) - pos, " | ");


			      for (uint32_t j = 0; j < 16; ++j) {
			          if ((i + j) < FLASH_PAGE_SIZE) {
			              uint8_t c = page_rx_buffer[i + j];

			              if (c >= 32 && c <= 126) {
			                  pos += snprintf(log_line + pos, sizeof(log_line) - pos, "%c", c);
			              } else {
			                  pos += snprintf(log_line + pos, sizeof(log_line) - pos, ".");
			              }
			          }
			      }


			      snprintf(log_line + pos, sizeof(log_line) - pos, "\r\n");


			      osDelay(TXFLASHLOG);
			      MSG_Enviar(log_line, TestUart_1);
			      osDelay(TXFLASHLOG);
			  }
		  }
	  }
   }




    osDelay(TXFLASHLOG);
	   MSG_Enviar(
		   "\r\n==========================================================\r\n"
		   " [SPI FLASH] [Flash_CMD_UART_Read_All_Sectors] UARD CMD - FIN \r\n"
		   "==========================================================\r\n",
		   TestUart_1
    );
	osDelay(TXFLASHLOG);


	Flash_Complement_Data_Thread(FLASH_DEBUG_THREAD_SIZE);

    return true;
}

