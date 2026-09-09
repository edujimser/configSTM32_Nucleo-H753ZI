#include <string.h>
#include <stdio.h>

#include "cmsis_os.h"

#include "SPI_Flash_Memory/SPI_Flash_Memory.h"
#include "MsgUART/TxUart/TxUart.h"


SPI_Flash_TypeDef spi_flash_memory_1;
extern I_TestUart TestUart_1;



bool Flash_Wait_For_Ready(SPI_Flash_TypeDef *flash, uint32_t timeout_ms)
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
            MSG_Enviar("\r\n [SPI FLASH ERROR] [Flash_Wait_For_Ready] Timeout de Mutex: El bus SPI esta ocupado.\r\n", TestUart_1);
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
                char buf_err[128];
                snprintf(buf_err, sizeof(buf_err),
                         "\r\n [SPI FLASH ERROR] [Flash_Wait_For_Ready] Fallo HAL_SPI (TX: %d, RX: %d).\r\n",
                         tx_status, rx_status);

                osDelay(TXFLASHLOG);
                MSG_Enviar(buf_err, TestUart_1);
                osDelay(TXFLASHLOG);
            }
            return false;
        }

        uint8_t bsy_state = status & 0x01;
        uint8_t wel_state = (status & 0x02) >> 1;
        uint8_t block_prot = (status & 0x1C) >> 2;


        if (block_prot > 0 && bsy_state == 0) {
            {
                char buf_warn[128];
                snprintf(buf_warn, sizeof(buf_warn),
                         " [SPI FLASH AVISO] [Flash_Wait_For_Ready] Bloques protegidos (BP bits: 0x%02X)\r\n",
                         block_prot);

                osDelay(TXFLASHLOG);
                MSG_Enviar(buf_warn, TestUart_1);
                osDelay(TXFLASHLOG);
            }
        }


        if (bsy_state == 0) {
            osDelay(TXFLASHLOG);
            MSG_Enviar(" [SPI FLASH] [Flash_Wait_For_Ready] Memoria lista (Libre de operaciones).\r\n", TestUart_1);
            osDelay(TXFLASHLOG);
            return true;
        }


        if ((HAL_GetTick() - last_print_tick) >= WAIT_FOR_READY_FAIL)
        {
            uint32_t seg_transcurridos = (HAL_GetTick() - start_tick) / 1000;
            {
                char buf_wait[64];
                snprintf(buf_wait, sizeof(buf_wait),
                         " [SPI FLASH] [Flash_Wait_For_Ready] Ocupada... Esperando (%lu s)\r\n",
                         seg_transcurridos);

                osDelay(TXFLASHLOG);
                MSG_Enviar(buf_wait, TestUart_1);
                osDelay(TXFLASHLOG);
            }
            last_print_tick = HAL_GetTick();
        }

        osDelay(100);
    }


    {
        char buf_timeout[128];
        snprintf(buf_timeout, sizeof(buf_timeout),
                 "\r\n [SPI FLASH ERROR] [Flash_Wait_For_Ready] Timeout excedido (%lu ms). Memoria bloqueada.\r\n",
                 timeout_ms);

        osDelay(TXFLASHLOG);
        MSG_Enviar(buf_timeout, TestUart_1);
        osDelay(TXFLASHLOG);
    }

    return false;
}

bool Flash_Write_Enable(SPI_Flash_TypeDef *flash)
{
    if (flash == NULL) {
        return false;
    }

    bool ready = Flash_Wait_For_Ready(flash, STATUS_FOR_READY);


    if (!ready) {
        osDelay(TXFLASHLOG);
        MSG_Enviar("\r\n [SPI FLASH ERROR] [Flash_Write_Enable] Memoria bloqueada, no se puede habilitar el Write Enable.\r\n", TestUart_1);
        osDelay(TXFLASHLOG);
        return false;
    }

    uint8_t cmd  = FLASH_CMD_WRITE_ENABLE;


    if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
        osDelay(TXFLASHLOG);
        MSG_Enviar("\r\n [SPI FLASH ERROR] [Flash_Write_Enable] Timeout de Mutex: El bus SPI esta ocupado.\r\n", TestUart_1);
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
                     "\r\n [SPI FLASH ERROR] [Flash_Write_Enable] Fallo HAL_SPI (TX: %d). Posible cable suelto o timeout.\r\n",
                     tx_status);

            osDelay(TXFLASHLOG);
            MSG_Enviar(buf_err, TestUart_1);
            osDelay(TXFLASHLOG);
        }
        return false;
    }


    osDelay(TXFLASHLOG);
    MSG_Enviar("\r\n [SPI FLASH] [Flash_Write_Enable] Habilitada escritura HAL_SPI .\r\n", TestUart_1);
    osDelay(TXFLASHLOG);

    return true;
}

bool Flash_Is_Sector_partition(SPI_Flash_TypeDef *flash, Flash_Partition_ID partition, uint32_t dir)
{
    if (flash == NULL) {
        return false;
    }

    char buf_msg[128];

    for (int i = 0; i < PARTITION_COUNT; ++i) {


        if (flash->partitions[i].id == partition) {

            if (dir >= flash->partitions[i].start_sector && dir <= flash->partitions[i].end_sector) {

                snprintf(buf_msg, sizeof(buf_msg), " [SPI FLASH] Sector %lu ENCONTRADO en la particion %d.\r\n",
                         (unsigned long)dir, (int)partition);

                osDelay(TXFLASHLOG);
                MSG_Enviar(buf_msg, TestUart_1);
                osDelay(TXFLASHLOG);

                return true;

            } else {

                snprintf(buf_msg, sizeof(buf_msg), " [SPI FLASH ERROR] Sector %lu FUERA DE LIMITES en particion %d.\r\n",
                         (unsigned long)dir, (int)partition);

                osDelay(TXFLASHLOG);
                MSG_Enviar(buf_msg, TestUart_1);
                osDelay(TXFLASHLOG);

                return false;
            }
        }
    }

    // Si el bucle termina sin encontrar el ID de la partición
    osDelay(TXFLASHLOG);
    MSG_Enviar(" [SPI FLASH ERROR] La particion solicitada NO EXISTE en el mapa.\r\n", TestUart_1);
    osDelay(TXFLASHLOG);
    return false;
}

bool Flash_HaveData_Sector(SPI_Flash_TypeDef *flash, Flash_Partition_ID partition, uint32_t sector_dir)
{
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

        uint32_t absolute_address = (sector_dir * 4096) + (page * 256);

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
                         (unsigned long)sector_dir, page, (b - 4), rx_unified[b]);

                osDelay(TXFLASHLOG);
                MSG_Enviar(buf_progreso, TestUart_1);
                osDelay(TXFLASHLOG);

                return true;
            }
        }


        osDelay(10);
    }


    snprintf(buf_progreso, sizeof(buf_progreso), " [SPI FLASH] Sector %lu completamente VACIO.\r\n", (unsigned long)sector_dir);
    osDelay(TXFLASHLOG);
    MSG_Enviar(buf_progreso, TestUart_1);
    osDelay(TXFLASHLOG);

    return false;
}
