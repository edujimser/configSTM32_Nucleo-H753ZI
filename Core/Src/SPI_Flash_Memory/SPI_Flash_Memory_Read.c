#include <string.h>
#include <stdio.h>

#include "cmsis_os.h"

#include "SPI_Flash_Memory/SPI_Flash_Memory.h"
#include "MsgUART/TxUart/TxUart.h"


extern I_TestUart TestUart_1;

bool Flash_Dump_All_Sectors(SPI_Flash_TypeDef *flash)
{
    if (flash == NULL) {
        return false;
    }

    osDelay(TXFLASHLOG);
    MSG_Enviar("\r\n [SPI FLASH] [Flash_Dump_All] Iniciando volcado y visualización de datos...\r\n", TestUart_1);
    osDelay(TXFLASHLOG);

    uint8_t page_rx_buffer[256];
    char buf_progreso[128];


    for (uint32_t sector = 0; sector < FLASH_SECTOR_COUNT; ++sector) {
            for (uint32_t page = 0; page < FLASH_NUM_PAGE; ++page) {

                uint32_t absolute_address = (sector * 4096) + (page * 256);

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


                snprintf(buf_progreso, sizeof(buf_progreso),
                         " [DUMP] Sec: %lu | Pág: %lu | Addr: 0x%06lX | Data[0]: 0x%02X Data[1]: 0x%02X\r\n",
                         (unsigned long)sector, (unsigned long)page, (unsigned long)absolute_address,

                         page_rx_buffer[0], page_rx_buffer[1]);
                osDelay(TXFLASHLOG);
                MSG_Enviar(buf_progreso, TestUart_1);
                osDelay(TXFLASHLOG);
            }
        }


    osDelay(TXFLASHLOG);
    MSG_Enviar(" [SPI FLASH] [Flash_Dump_All] Volcado visual completado con exito.\r\n", TestUart_1);
    osDelay(TXFLASHLOG);


    {
        char *nombre_tarea = (char*)pcTaskGetName(NULL);
        UBaseType_t libres_words = uxTaskGetStackHighWaterMark(NULL);
        uint32_t libres_bytes = (uint32_t)(libres_words * 4);

        char buf_memoria[128];
        snprintf(buf_memoria, sizeof(buf_memoria),
                 "\r\n [DIAGNOSTICO] [Flash_Dump_All]\r\n"
                 "  |- Tarea Activa : %s\r\n"
                 "  \\- Margen Libre : %lu bytes\r\n",
                 nombre_tarea,
                 (unsigned long)libres_bytes);

        MSG_Enviar(buf_memoria, TestUart_1);
    }

    return true;
}

bool Flash_Dump_One_Sectors_Hex(SPI_Flash_TypeDef *flash, uint32_t sector)
{
    if (flash == NULL) {
        return false;
    }

    osDelay(TXFLASHLOG);
    MSG_Enviar("\r\n [SPI FLASH] [Flash_Dump_All] Iniciando volcado y visualización de datos...\r\n", TestUart_1);
    osDelay(TXFLASHLOG);


    char buf_progreso[128];


	for (uint32_t page = 0; page < FLASH_NUM_PAGE; ++page) {

		uint32_t absolute_address = (sector * 4096) + (page * 256);

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


	    uint8_t page_rx_buffer[256];
		memcpy(page_rx_buffer, &rx_unified[4], 256);


		snprintf(buf_progreso, sizeof(buf_progreso),
		         "\r\n [DUMP] Sec: %lu | Pág: %lu | Addr Base: 0x%06lX\r\n",
		         (unsigned long)sector, (unsigned long)page, (unsigned long)absolute_address);

		osDelay(TXFLASHLOG);
		MSG_Enviar(buf_progreso, TestUart_1);
		osDelay(TXFLASHLOG);

		for (uint32_t i = 0; i < 256; i += 16) {

		    snprintf(buf_progreso, sizeof(buf_progreso),
		             "  [+%03X] %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
		             (unsigned int)i,
		             page_rx_buffer[i+0],  page_rx_buffer[i+1],  page_rx_buffer[i+2],  page_rx_buffer[i+3],
		             page_rx_buffer[i+4],  page_rx_buffer[i+5],  page_rx_buffer[i+6],  page_rx_buffer[i+7],
		             page_rx_buffer[i+8],  page_rx_buffer[i+9],  page_rx_buffer[i+10], page_rx_buffer[i+11],
		             page_rx_buffer[i+12], page_rx_buffer[i+13], page_rx_buffer[i+14], page_rx_buffer[i+15]);

			osDelay(TXFLASHLOG);
		    MSG_Enviar(buf_progreso, TestUart_1);
			osDelay(TXFLASHLOG);
		}
   }



    osDelay(TXFLASHLOG);
    MSG_Enviar(" [SPI FLASH] [Flash_Dump_All] Volcado visual completado con exito.\r\n", TestUart_1);
    osDelay(TXFLASHLOG);


    {
        char *nombre_tarea = (char*)pcTaskGetName(NULL);
        UBaseType_t libres_words = uxTaskGetStackHighWaterMark(NULL);
        uint32_t libres_bytes = (uint32_t)(libres_words * 4);

        char buf_memoria[128];
        snprintf(buf_memoria, sizeof(buf_memoria),
                 "\r\n [DIAGNOSTICO] [Flash_Dump_All]\r\n"
                 "  |- Tarea Activa : %s\r\n"
                 "  \\- Margen Libre : %lu bytes\r\n",
                 nombre_tarea,
                 (unsigned long)libres_bytes);

        osDelay(TXFLASHLOG);
        MSG_Enviar(buf_memoria, TestUart_1);
        osDelay(TXFLASHLOG);
    }

    return true;
}

bool Flash_Dump_One_Sectors_ASCII(SPI_Flash_TypeDef *flash, uint32_t sector)
{
    if (flash == NULL) {
        return false;
    }

    MSG_Enviar("\r\n [SPI FLASH] Iniciando volcado HEX + ASCII...\r\n", TestUart_1);

    char buf_progreso[128];

    for (uint32_t page = 0; page < FLASH_NUM_PAGE; ++page) {

        uint32_t absolute_address = (sector * 4096) + (page * 256);

        uint8_t tx_unified[260];
        uint8_t rx_unified[260];

        memset(tx_unified, 0, 260);
        memset(rx_unified, 0, 260);

        tx_unified[0] = FLASH_CMD_READ;
        tx_unified[1] = (uint8_t)((absolute_address >> 16) & 0xFF);
        tx_unified[2] = (uint8_t)((absolute_address >> 8) & 0xFF);
        tx_unified[3] = (uint8_t)(absolute_address & 0xFF);


        if (osMutexAcquire(flash->spi_mutex, pdMS_TO_TICKS(1000)) != osOK) {
            MSG_Enviar("\r\n [SPI FLASH ERROR] Timeout de Mutex en lectura.\r\n", TestUart_1);
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


        uint8_t page_rx_buffer[256];
        memcpy(page_rx_buffer, &rx_unified[4], 256);


        snprintf(buf_progreso, sizeof(buf_progreso),
                 "\r\n [DUMP] Sec: %lu | Pág: %lu | Addr Base: 0x%06lX\r\n",
                 (unsigned long)sector, (unsigned long)page, (unsigned long)absolute_address);
        MSG_Enviar(buf_progreso, TestUart_1);


        for (uint32_t i = 0; i < 256; i += 16) {


            char ascii_str[17];
            for (int j = 0; j < 16; j++) {
                uint8_t byte_val = page_rx_buffer[i + j];

                if (byte_val >= 32 && byte_val <= 126) {
                    ascii_str[j] = (char)byte_val;
                } else {
                    ascii_str[j] = '.';
                }
            }
            ascii_str[16] = '\0';


            snprintf(buf_progreso, sizeof(buf_progreso),
                     "  [+%03X] %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X | %s\r\n",
                     (unsigned int)i,
                     page_rx_buffer[i+0],  page_rx_buffer[i+1],  page_rx_buffer[i+2],  page_rx_buffer[i+3],
                     page_rx_buffer[i+4],  page_rx_buffer[i+5],  page_rx_buffer[i+6],  page_rx_buffer[i+7],
                     page_rx_buffer[i+8],  page_rx_buffer[i+9],  page_rx_buffer[i+10], page_rx_buffer[i+11],
                     page_rx_buffer[i+12], page_rx_buffer[i+13], page_rx_buffer[i+14], page_rx_buffer[i+15],
                     ascii_str);

            osDelay(TXFLASHLOG);
            MSG_Enviar(buf_progreso, TestUart_1);
            osDelay(TXFLASHLOG);
        }
    }

    MSG_Enviar("\r\n [SPI FLASH] Volcado HEX+ASCII completado con exito.\r\n", TestUart_1);


    {
        char buf_memoria[128];
        snprintf(buf_memoria, sizeof(buf_memoria),
                 " [DIAGNOSTICO] %s | Margen Libre: %lu bytes\r\n",
                 pcTaskGetName(NULL), (unsigned long)(uxTaskGetStackHighWaterMark(NULL) * 4));
        osDelay(TXFLASHLOG);
        MSG_Enviar(buf_memoria, TestUart_1);
        osDelay(TXFLASHLOG);
    }

    return true;
}

bool Flash_Dump_One_Page_Hex(SPI_Flash_TypeDef *flash, uint32_t sector, uint8_t page)
{
    if (flash == NULL) {
        return false;
    }

    osDelay(TXFLASHLOG);
    MSG_Enviar("\r\n [SPI FLASH] [Flash_Dump_One_Page_Hex] Iniciando volcado...\r\n", TestUart_1);
    osDelay(TXFLASHLOG);

    if(page < 0 || page > FLASH_NUM_PAGE){
		osDelay(TXFLASHLOG);
		MSG_Enviar("\r\n [SPI FLASH] [Flash_Dump_One_Page_Hex] Superado Limite de pagina\r\n", TestUart_1);
		osDelay(TXFLASHLOG);

		return false;
	}

	if(sector < 0 || sector > FLASH_SECTOR_COUNT){
		osDelay(TXFLASHLOG);
		MSG_Enviar("\r\n [SPI FLASH] [Flash_Dump_One_Page_Hex] Superado Limite de sectores\r\n", TestUart_1);
		osDelay(TXFLASHLOG);

		return false;
	}


	uint32_t absolute_address = (sector * 4096) + (page * 256);

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


	uint8_t page_rx_buffer[256];
	memcpy(page_rx_buffer, &rx_unified[4], 256);

	char buf_progreso[128];
	snprintf(buf_progreso, sizeof(buf_progreso),
			 "\r\n [DUMP] Sec: %lu | Pág: %lu | Addr Base: 0x%06lX\r\n",
			 (unsigned long)sector, (unsigned long)page, (unsigned long)absolute_address);

	osDelay(TXFLASHLOG);
	MSG_Enviar(buf_progreso, TestUart_1);
	osDelay(TXFLASHLOG);

	for (uint32_t i = 0; i < 256; i += 16) {

		snprintf(buf_progreso, sizeof(buf_progreso),
				 "  [+%03X] %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X\r\n",
				 (unsigned int)i,
				 page_rx_buffer[i+0],  page_rx_buffer[i+1],  page_rx_buffer[i+2],  page_rx_buffer[i+3],
				 page_rx_buffer[i+4],  page_rx_buffer[i+5],  page_rx_buffer[i+6],  page_rx_buffer[i+7],
				 page_rx_buffer[i+8],  page_rx_buffer[i+9],  page_rx_buffer[i+10], page_rx_buffer[i+11],
				 page_rx_buffer[i+12], page_rx_buffer[i+13], page_rx_buffer[i+14], page_rx_buffer[i+15]);

		osDelay(TXFLASHLOG);
		MSG_Enviar(buf_progreso, TestUart_1);
		osDelay(TXFLASHLOG);
	}




    osDelay(TXFLASHLOG);
    MSG_Enviar(" [SPI FLASH] [Flash_Dump_All] Volcado visual completado con exito.\r\n", TestUart_1);
    osDelay(TXFLASHLOG);


    {
        char *nombre_tarea = (char*)pcTaskGetName(NULL);
        UBaseType_t libres_words = uxTaskGetStackHighWaterMark(NULL);
        uint32_t libres_bytes = (uint32_t)(libres_words * 4);

        char buf_memoria[128];
        snprintf(buf_memoria, sizeof(buf_memoria),
                 "\r\n [DIAGNOSTICO] [Flash_Dump_All]\r\n"
                 "  |- Tarea Activa : %s\r\n"
                 "  \\- Margen Libre : %lu bytes\r\n",
                 nombre_tarea,
                 (unsigned long)libres_bytes);

        osDelay(TXFLASHLOG);
        MSG_Enviar(buf_memoria, TestUart_1);
        osDelay(TXFLASHLOG);
    }

    return true;
}

bool Flash_Dump_One_Page_ASCII(SPI_Flash_TypeDef *flash, uint32_t sector, uint8_t page)
{
    if (flash == NULL) {
        return false;
    }

    osDelay(TXFLASHLOG);
    MSG_Enviar("\r\n [Flash_Dump_One_Page_ASCII] Iniciando volcado ASCII...\r\n", TestUart_1);
    osDelay(TXFLASHLOG);

    if(page < 0 || page > FLASH_NUM_PAGE){
    	osDelay(TXFLASHLOG);
    	MSG_Enviar("\r\n [SPI FLASH] [Flash_Dump_One_Page_ASCII] Superado Limite de pagina\r\n", TestUart_1);
		osDelay(TXFLASHLOG);

		return false;
    }

    if(sector < 0 || sector > FLASH_SECTOR_COUNT){
    	osDelay(TXFLASHLOG);
    	MSG_Enviar("\r\n [SPI FLASH] [Flash_Dump_One_Page_ASCII] Superado Limite de sectores\r\n", TestUart_1);
		osDelay(TXFLASHLOG);

		return false;
    }

	uint32_t absolute_address = (sector * 4096) + (page * 256);

	uint8_t tx_unified[260];
	uint8_t rx_unified[260];

	memset(tx_unified, 0, 260);
	memset(rx_unified, 0, 260);

	tx_unified[0] = FLASH_CMD_READ;
	tx_unified[1] = (uint8_t)((absolute_address >> 16) & 0xFF);
	tx_unified[2] = (uint8_t)((absolute_address >> 8) & 0xFF);
	tx_unified[3] = (uint8_t)(absolute_address & 0xFF);


	if (osMutexAcquire(flash->spi_mutex, pdMS_TO_TICKS(1000)) != osOK) {
		MSG_Enviar("\r\n [SPI FLASH ERROR] Timeout de Mutex en lectura.\r\n", TestUart_1);
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


	uint8_t page_rx_buffer[256];
	memcpy(page_rx_buffer, &rx_unified[4], 256);

	char buf_progreso[128];

	snprintf(buf_progreso, sizeof(buf_progreso),
			 "\r\n [DUMP] Sec: %lu | Pág: %lu | Addr Base: 0x%06lX\r\n",
			 (unsigned long)sector, (unsigned long)page, (unsigned long)absolute_address);
	MSG_Enviar(buf_progreso, TestUart_1);


	for (uint32_t i = 0; i < 256; i += 16) {


		char ascii_str[17];
		for (int j = 0; j < 16; j++) {
			uint8_t byte_val = page_rx_buffer[i + j];

			if (byte_val >= 32 && byte_val <= 126) {
				ascii_str[j] = (char)byte_val;
			} else {
				ascii_str[j] = '.';
			}
		}
		ascii_str[16] = '\0';


		snprintf(buf_progreso, sizeof(buf_progreso),
				 "  [+%03X] %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X %02X | %s\r\n",
				 (unsigned int)i,
				 page_rx_buffer[i+0],  page_rx_buffer[i+1],  page_rx_buffer[i+2],  page_rx_buffer[i+3],
				 page_rx_buffer[i+4],  page_rx_buffer[i+5],  page_rx_buffer[i+6],  page_rx_buffer[i+7],
				 page_rx_buffer[i+8],  page_rx_buffer[i+9],  page_rx_buffer[i+10], page_rx_buffer[i+11],
				 page_rx_buffer[i+12], page_rx_buffer[i+13], page_rx_buffer[i+14], page_rx_buffer[i+15],
				 ascii_str);

		osDelay(TXFLASHLOG);
		MSG_Enviar(buf_progreso, TestUart_1);
		osDelay(TXFLASHLOG);
	}

    MSG_Enviar("\r\n [SPI FLASH] Volcado HEX+ASCII completado con exito.\r\n", TestUart_1);


    {
        char buf_memoria[128];
        snprintf(buf_memoria, sizeof(buf_memoria),
                 " [DIAGNOSTICO] %s | Margen Libre: %lu bytes\r\n",
                 pcTaskGetName(NULL), (unsigned long)(uxTaskGetStackHighWaterMark(NULL) * 4));
        osDelay(TXFLASHLOG);
        MSG_Enviar(buf_memoria, TestUart_1);
        osDelay(TXFLASHLOG);
    }

    return true;
}


