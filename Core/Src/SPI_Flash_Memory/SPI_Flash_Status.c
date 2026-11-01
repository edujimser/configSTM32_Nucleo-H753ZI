#include <string.h>
#include <stdio.h>

#include "cmsis_os.h"

#include "SPI_Flash_Memory/SPI_Flash_Memory.h"
#include "MsgUART/TxUart/TxUart.h"


extern I_TestUart TestUart_1;


void Flash_Read_ID(SPI_Flash_TypeDef *flash)
{
    if (flash == NULL || flash->spi_mutex == NULL || flash->hspi == NULL || flash->cs_port == NULL) {
        return;
    }

    uint8_t cmd = FLASH_CMD_ID;
    uint8_t id_rx[3] = {0};


    if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
        osDelay(TXFLASHLOG);
        MSG_Enviar("\r\n [SPI FLASH ERROR] [Flash_Read_ID] Timeout de Mutex: El bus SPI esta ocupado.\r\n", TestUart_1);
        osDelay(TXFLASHLOG);
        return;
    }


    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef tx_status = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
    HAL_StatusTypeDef rx_status = HAL_SPI_Receive(flash->hspi, id_rx, 3, WRITE_READ_NORMAL_VELOCITY);
    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

    osMutexRelease(flash->spi_mutex);


    if (tx_status != HAL_OK || rx_status != HAL_OK) {
        osDelay(TXFLASHLOG);
        MSG_Enviar("\r\n [SPI FLASH ERROR] [Flash_Read_ID] HAL_SPI ha devuelto un error (Timeout o Busy).\r\n", TestUart_1);
        osDelay(TXFLASHLOG);
        return;
    }

    if (id_rx[0] == 0x00 || id_rx[0] == 0xFF) {
        if (id_rx[0] == 0x00) {
            osDelay(TXFLASHLOG);
            MSG_Enviar("\r\n [SPI FLASH ERROR] [Flash_Read_ID] Fallo comunicacion SPI.\r\n └─ ID: 0x00 (Posible corto a GND o chip dañado).\r\n\r\n", TestUart_1);
            osDelay(TXFLASHLOG);
            return;
        }

        if (id_rx[0] == 0xFF) {
            osDelay(TXFLASHLOG);
            MSG_Enviar("\r\n [SPI FLASH ERROR] [Flash_Read_ID] Fallo comunicacion SPI.\r\n └─ ID: 0xFF (Chip ausente o pines desoldados).\r\n\r\n", TestUart_1);
            osDelay(TXFLASHLOG);
            return;
        }
    }

    const char *fabricante = "Desconocido";
    const char *capacidad = "Desconocida";

    if (id_rx[0] == 0xC8) {
        fabricante = "GigaDevice (GD25Q...)";
    }
    else if (id_rx[0] == 0xEF) {
        fabricante = "Winbond (W25Q...)";
    }
    else if (id_rx[0] == 0xC2) {
        fabricante = "Macronix (MX25L...)";
    }

    if (id_rx[2] == 0x18) {
        capacidad = "16 MB (128 Mbit)";
    }
    else if (id_rx[2] == 0x17) {
        capacidad = "8 MB (64 Mbit)";
    }


    {

        char msn_buffer_local[256];

        snprintf(msn_buffer_local, sizeof(msn_buffer_local),
                 "\r\n [SPI FLASH] [Flash_Read_ID] Analisis de Hardware Exitoso:\r\n"
                 " ├─ JEDEC ID Raw : %02X %02X %02X\r\n"
                 " ├─ Fabricante   : %s\r\n"
                 " └─ Capacidad    : %s\r\n",
                 id_rx[0], id_rx[1], id_rx[2],
                 fabricante,
                 capacidad);

        if (!MSG_Enviar_Longitud_Limit(msn_buffer_local)) {
            MSG_Enviar(msn_buffer_local, TestUart_1);
        }
        osDelay(TXFLASHLOG);
    }
}


void Flash_Read_Component(SPI_Flash_TypeDef *flash)
{

    if (flash == NULL || flash->hspi == NULL || flash->cs_port == NULL || flash->spi_mutex == NULL) {
        osDelay(TXFLASHLOG);
        MSG_Enviar("\r\n [SPI FLASH ERROR] [Flash_Read_Component] Puntero nulo detectado en el componente.\r\n", TestUart_1);
        osDelay(TXFLASHLOG);
        return;
    }


    if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
        osDelay(TXFLASHLOG);
        MSG_Enviar("\r\n [SPI FLASH ERROR] [Flash_Read_Component] Timeout de Mutex: El bus SPI esta ocupado.\r\n", TestUart_1);
        osDelay(TXFLASHLOG);
        return;
    }

    uint8_t cmd_id = FLASH_CMD_ID;
    uint8_t cmd_status = FLASH_CMD_READ_STATUS;
    uint8_t id_rx[3] = {0};
    uint8_t status_reg = 0;


    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef tx1 = HAL_SPI_Transmit(flash->hspi, &cmd_id, 1, WRITE_READ_NORMAL_VELOCITY);
    HAL_StatusTypeDef rx1 = HAL_SPI_Receive(flash->hspi, id_rx, 3, WRITE_READ_NORMAL_VELOCITY);
    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

    for(volatile int i = 0; i < 200; i++);

    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef tx2 = HAL_SPI_Transmit(flash->hspi, &cmd_status, 1, WRITE_READ_NORMAL_VELOCITY);
    HAL_StatusTypeDef rx2 = HAL_SPI_Receive(flash->hspi, &status_reg, 1, WRITE_READ_NORMAL_VELOCITY);
    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

    osMutexRelease(flash->spi_mutex);


    if (tx1 != HAL_OK || rx1 != HAL_OK || tx2 != HAL_OK || rx2 != HAL_OK) {
        osDelay(TXFLASHLOG);
        MSG_Enviar("\r\n [SPI FLASH ERROR] [Flash_Read_Component] Error de comunicacion HAL_SPI.\r\n", TestUart_1);
        osDelay(TXFLASHLOG);
        return;
    }

    uint8_t bsy = (status_reg & 0x01);
    uint8_t wel = (status_reg & 0x02) >> 1;

    {
        char msn_buffer_local[256];

        snprintf(msn_buffer_local, sizeof(msn_buffer_local),
                 "\r\n=======================\r\n"
                 " [SPI FLASH] ESTADO \r\n"
                 "=======================\r\n"
                 "  - JEDEC ID Raw     : %02hhX %02hhX %02hhX\r\n",
                 id_rx[0], id_rx[1], id_rx[2]);
        osDelay(TXFLASHLOG);
        MSG_Enviar(msn_buffer_local, TestUart_1);
        osDelay(TXFLASHLOG);

        snprintf(msn_buffer_local, sizeof(msn_buffer_local),
                 "  - Estado (Reg-1)   : 0x%02X\r\n"
                 "    * BSY (Ocupado)  : %d\r\n"
                 "    * WEL (Escritura): %d\r\n"
                 "  - Pines CS         : Port %p (Pin 0x%04X)\r\n"
                 "  - Estado Particiones:\r\n",
                 status_reg, bsy, wel,
                 (void*)flash->cs_port, flash->cs_pin);
        osDelay(TXFLASHLOG);
        MSG_Enviar(msn_buffer_local, TestUart_1);
        osDelay(TXFLASHLOG);
    }


    {
        char buffer_particion[128];

        for (int i = 0; i < FLASH_SECTION_COUNT_END; i++)
        {
            if (flash->partitions[i].configured)
            {
                snprintf(buffer_particion, sizeof(buffer_particion),
                         "    -> Particion [%d] : Sectores %l4 a %lu\r\n"
                         "       - Sector Actual : %lu\r\n"
                         "       - Desplazamiento: %lu Bytes\r\n",
                         i,
                         flash->partitions[i].start_sector,
                         flash->partitions[i].end_sector,
                         flash->partitions[i].current_sector,
                         flash->partitions[i].sector_offset);

                osDelay(TXFLASHLOG);
                MSG_Enviar(buffer_particion, TestUart_1);
                osDelay(TXFLASHLOG);
            }
        }
    }


    osDelay(TXFLASHLOG);
    MSG_Enviar("========================================\r\n\r\n", TestUart_1);
    osDelay(TXFLASHLOG);
}
