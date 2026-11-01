#include <string.h>
#include <stdio.h>

#include "cmsis_os.h"

#include "SPI_Flash_Memory/SPI_Flash_Memory.h"
#include "MsgUART/TxUart/TxUart.h"

extern I_TestUart TestUart_1;

void Flash_Complement_Data_Thread(bool debugSystem){

	char *nombre_tarea = (char*)pcTaskGetName(NULL);
	UBaseType_t libres_words = uxTaskGetStackHighWaterMark(NULL);
	uint32_t libres_bytes = (uint32_t)(libres_words * 4);

	if(!debugSystem && (libres_bytes > FLASH_DEBUG_THREAD_SIZE_LIMIT)) return;

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

bool Flash_Comeplement_Wait_For_Ready(SPI_Flash_TypeDef *flash, uint32_t timeout_ms)
{
    if (flash == NULL) {
        return false;
    }

    uint8_t cmd = FLASH_CMD_READ_STATUS;
    uint8_t status = 0;
    uint32_t start_tick = HAL_GetTick();
    uint32_t last_print_tick = HAL_GetTick();

    while ((HAL_GetTick() - start_tick) < timeout_ms)
    {

        if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
            osDelay(TXFLASHLOG);
            MSG_Enviar("\r\n [SPI FLASH ERROR] [Flash_Comeplement_Wait_For_Ready] Timeout de Mutex: El bus SPI esta ocupado.\r\n", TestUart_1);
            osDelay(TXFLASHLOG);
            return false;
        }


        HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
        HAL_StatusTypeDef tx_status = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
        HAL_StatusTypeDef rx_status = HAL_SPI_Receive(flash->hspi, &status, 1, WRITE_READ_NORMAL_VELOCITY);
        HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

        osMutexRelease(flash->spi_mutex);


        if (tx_status != HAL_OK || rx_status != HAL_OK) {
            {
                char buf_err[256];
                snprintf(buf_err, sizeof(buf_err),
                         "\r\n [SPI FLASH ERROR] [Flash_Comeplement_Wait_For_Ready] Fallo HAL_SPI (TX: %d, RX: %d).\r\n",
                         tx_status, rx_status);

                osDelay(TXFLASHLOG);
                MSG_Enviar(buf_err, TestUart_1);
                osDelay(TXFLASHLOG);
            }
            return false;
        }

        uint8_t bsy_state = status & 0x01;
        uint8_t block_prot = (status & 0x1C) >> 2;


        if (block_prot > 0 && bsy_state == 0) {
            {
                char buf_warn[256];
                snprintf(buf_warn, sizeof(buf_warn),
                         " \r\n[SPI FLASH AVISO] [Flash_Comeplement_Wait_For_Ready] Bloques protegidos (BP bits: 0x%02X)\r\n",
                         block_prot);

                osDelay(TXFLASHLOG);
                MSG_Enviar(buf_warn, TestUart_1);
                osDelay(TXFLASHLOG);
            }
        }


        if (bsy_state == 0) {
            osDelay(TXFLASHLOG);
            MSG_Enviar(" \r\n [SPI FLASH] [Flash_Comeplement_Wait_For_Ready] Memoria lista (Libre de operaciones).\r\n", TestUart_1);
            osDelay(TXFLASHLOG);
            return true;
        }


        if ((HAL_GetTick() - last_print_tick) >= WAIT_FOR_READY_FAIL)
        {
            uint32_t seg_transcurridos = (HAL_GetTick() - start_tick) / 1000;
            {
                char buf_wait[125];
                snprintf(buf_wait, sizeof(buf_wait),
                         " [SPI FLASH] [Flash_Comeplement_Wait_For_Ready] Ocupada... Esperando (%lu s)\r\n",
                         seg_transcurridos);

                osDelay(TXFLASHLOG);
                MSG_Enviar(buf_wait, TestUart_1);
                osDelay(TXFLASHLOG);
            }
            last_print_tick = HAL_GetTick();
        }

        osDelay(10);
    }


    {
        char buf_timeout[128];
        snprintf(buf_timeout, sizeof(buf_timeout),
                 "\r\n [SPI FLASH ERROR] [Flash_Comeplement_Wait_For_Ready] Timeout excedido (%lu ms). Memoria bloqueada.\r\n",
                 timeout_ms);

        osDelay(TXFLASHLOG);
        MSG_Enviar(buf_timeout, TestUart_1);
        osDelay(TXFLASHLOG);
    }

    return false;
}

bool Flash_Complement_Write_Enable(SPI_Flash_TypeDef *flash)
{
    if (flash == NULL) {
        return false;
    }

    bool ready = Flash_Comeplement_Wait_For_Ready(flash, STATUS_FOR_READY);


    if (!ready) {
        osDelay(TXFLASHLOG);
        MSG_Enviar("\r\n [SPI FLASH ERROR] [Flash_Complement_Write_Enable] Memoria bloqueada, no se puede habilitar el Write Enable.\r\n", TestUart_1);
        osDelay(TXFLASHLOG);
        return false;
    }

    uint8_t cmd  = FLASH_CMD_WRITE_ENABLE;


    if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
        osDelay(TXFLASHLOG);
        MSG_Enviar("\r\n [SPI FLASH ERROR] [Flash_Complement_Write_Enable] Timeout de Mutex: El bus SPI esta ocupado.\r\n", TestUart_1);
        osDelay(TXFLASHLOG);
        return false;
    }

    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef tx_status = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

    osMutexRelease(flash->spi_mutex);


    if (tx_status != HAL_OK) {
        {
            char buf_err[128];
            snprintf(buf_err, sizeof(buf_err),
                     "\r\n [SPI FLASH ERROR] [Flash_Complement_Write_Enable] Fallo HAL_SPI (TX: %d). Posible cable suelto o timeout.\r\n",
                     tx_status);

            osDelay(TXFLASHLOG);
            MSG_Enviar(buf_err, TestUart_1);
            osDelay(TXFLASHLOG);
        }
        return false;
    }


    osDelay(TXFLASHLOG);
    MSG_Enviar("\r\n [SPI FLASH] [Flash_Complement_Write_Enable] Habilitada escritura HAL_SPI .\r\n", TestUart_1);
    osDelay(TXFLASHLOG);

    return true;
}

bool Flash_Complement_Is_Sector_partition(SPI_Flash_TypeDef *flash, Flash_Partition_ID partition, uint32_t addr)
{
    if (flash == NULL) {
        return false;
    }

    char buf_msg[128];

    for (int i = 0; i < FLASH_SECTION_COUNT_END; ++i) {


        if (flash->partitions[i].id == partition) {

            if (addr >= flash->partitions[i].start_sector && addr <= flash->partitions[i].end_sector) {

                snprintf(buf_msg, sizeof(buf_msg), " [SPI FLASH] Sector %lu ENCONTRADO en la particion %d.\r\n",
                         (unsigned long)addr, (int)partition);

                osDelay(TXFLASHLOG);
                MSG_Enviar(buf_msg, TestUart_1);
                osDelay(TXFLASHLOG);

                return true;

            } else {

                snprintf(buf_msg, sizeof(buf_msg), " [SPI FLASH ERROR] Sector %lu FUERA DE LIMITES en particion %d.\r\n",
                         (unsigned long)addr, (int)partition);

                osDelay(TXFLASHLOG);
                MSG_Enviar(buf_msg, TestUart_1);
                osDelay(TXFLASHLOG);

                return false;
            }
        }
    }

    osDelay(TXFLASHLOG);
    MSG_Enviar(" [SPI FLASH ERROR] La particion solicitada NO EXISTE en el mapa.\r\n", TestUart_1);
    osDelay(TXFLASHLOG);
    return false;
}



bool Flash_Complement_HaveData_Sector(SPI_Flash_TypeDef *flash, Flash_Partition_ID partition, uint32_t addr){
    if (flash == NULL) {
        return false;
    }


    uint8_t pages = FLASH_SECTION_SIZE / FLASH_PAGE_SIZE;
    uint8_t tx_unified[260];
    uint8_t rx_unified[260];
    char buf_progreso[128];


    for (int page = 0; page < pages; ++page) {

        memset(tx_unified, 0, 260);
        memset(rx_unified, 0, 260);

        uint32_t absolute_address = addr + (page * 256);

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

        osDelay(50);

        if (estado_spi != HAL_OK) {
        	osDelay(TXFLASHLOG);
            MSG_Enviar("\r\n [SPI FLASH ERROR] Fallo hardware en lectura de datos.\r\n", TestUart_1);
            osDelay(TXFLASHLOG);
            return false;
        }

        for (int b = 4; b < 260; ++b) {
            if (rx_unified[b] != 0xFF) {

                snprintf(buf_progreso, sizeof(buf_progreso),
                         " [SPI FLASH] Sector %lu TIENE DATOS (Pág %d, Byte %d = 0x%02X)\r\n",
                         (unsigned long)addr, page, (b - 4), rx_unified[b]);

                osDelay(TXFLASHLOG);
                MSG_Enviar(buf_progreso, TestUart_1);
                osDelay(TXFLASHLOG);

                return true;
            }
        }


        osDelay(10);
    }


    snprintf(buf_progreso, sizeof(buf_progreso), " [SPI FLASH] Sector %lu completamente VACIO.\r\n", (unsigned long)addr);
    osDelay(TXFLASHLOG);
    MSG_Enviar(buf_progreso, TestUart_1);
    osDelay(TXFLASHLOG);

    return false;
}

bool Flash_Complement_Ocuped(SPI_Flash_TypeDef *flash, char forcedFree){
	if(forcedFree){
		flash->ocuped = false;
	}else{
		if(flash->ocuped){
			 return true;
		}else{
			flash->ocuped = true;
		}
	}

	return false;
}

bool Flash_Wait_Busy(SPI_Flash_TypeDef *flash, uint32_t timeout_ms, uint32_t *timeUse) {
    uint8_t cmd_read_status = 0x05;
    uint8_t status_reg = 0;
    uint32_t start_tick = HAL_GetTick();

    do {

        HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);


        HAL_SPI_Transmit(flash->hspi, &cmd_read_status, 1, 100);

        HAL_SPI_Receive(flash->hspi, &status_reg, 1, 100);

        HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);


        if ((status_reg & 0x01) == 0) {
            if (timeUse != NULL) {
                *timeUse = HAL_GetTick() - start_tick;
            }
            return true; // El chip ha terminado
        }


        osDelay(5);

    } while ((HAL_GetTick() - start_tick) < timeout_ms);


    return false;
}

bool Flash_Complement_protetionLimit(SPI_Flash_TypeDef *flash, uint32_t addr) {
	uint32_t start_addr = flash->partitions[FLASH_SECTION_LOG_MONITOR].start_sector ;
	uint32_t end_addr   = flash->partitions[FLASH_SECTION_LOG_MONITOR].end_sector;

	if(addr < start_addr || addr > end_addr) {
		return false;
	}
	return true;
}

bool Flash_Complement_protetionLimit_Range(SPI_Flash_TypeDef *flash, uint32_t addrInit, uint32_t addrFinal) {
	uint32_t start_addr = flash->partitions[FLASH_SECTION_LOG_MONITOR].start_sector ;
	uint32_t end_addr   = flash->partitions[FLASH_SECTION_LOG_MONITOR].end_sector;

	if(addrInit < start_addr || addrFinal > end_addr) {
		return false;
	}
	return true;
}

