#include <string.h>
#include <stdio.h>

#include "cmsis_os.h"

#include "SPI_Flash_Memory/SPI_Flash_Memory.h"
#include "MsgUART/TxUart/TxUart.h"


extern I_TestUart TestUart_1;




void Flash_Init(SPI_Flash_TypeDef *flash, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin, bool mensaje)
{
    char msn_buffer[256]; // Buffer unificado a 256 bytes

    if (flash == NULL || hspi == NULL || cs_port == NULL) {
        snprintf(msn_buffer, sizeof(msn_buffer),
                 "\r\n [SPI FLASH ERROR CRITICO] [%s] Puntero nulo detectado en Init.\r\n", __func__);
        MSG_Enviar(msn_buffer, TestUart_1);
        return;
    }

    flash->hspi = hspi;
    flash->cs_port = cs_port;
    flash->cs_pin = cs_pin;
    flash->ocuped = false;

    if (flash->spi_mutex == NULL) {
        flash->spi_mutex = osMutexNew(NULL);
        if (flash->spi_mutex == NULL) {
            snprintf(msn_buffer, sizeof(msn_buffer),
                     "\r\n [SPI ERROR] [%s] No se pudo crear el Mutex del SPI.\r\n", __func__);
            MSG_Enviar(msn_buffer, TestUart_1);
            return;
       }
    }

    // Retardo generoso para permitir que la alimentación de la Flash se estabilice por completo
    osDelay(50);

    // =========================================================================
    // 1. RESET DE HARDWARE POR SOFTWARE (Fuerza modo SPI estándar y limpia registros)
    // =========================================================================
    uint8_t cmd_reset_enable = 0x66;
    uint8_t cmd_reset = 0x99;

    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(flash->hspi, &cmd_reset_enable, 1, 100);
    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);
    osDelay(5);

    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(flash->hspi, &cmd_reset, 1, 100);
    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);
    osDelay(50); // Margen amplio para el rearme interno de la memoria
    // =========================================================================

    // Detección inicial del chip (Lectura de ID)
    if (!Flash_detect_chip_Init(flash)) {
        return;
    }

    // =========================================================================
    // 2. DESBLOQUEO GLOBAL DE HARDWARE (Elimina protección de fábrica en zona alta)
    // =========================================================================
    if (!Flash_Write_Enable(flash)) {
        if (mensaje) {
            MSG_Enviar("\r\n [SPI FLASH ERROR] [%s] Fallo en Write Enable para Desbloqueo Global.\r\n", TestUart_1);
        }
        return;
    }

    uint8_t cmd_unprotect = 0x01; // Comando Write Status Register
    uint8_t status_val[2] = {0x00, 0x00}; // Todos los bits BP a 0 (Sin protecciones)

    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(flash->hspi, &cmd_unprotect, 1, 100);
    HAL_SPI_Transmit(flash->hspi, status_val, 2, 100);
    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

    osDelay(20);

    // Esperar a que el chip procese internamente el desbloqueo global mediante el bit BSY
    if (!Flash_Wait_For_Ready(flash, 500)) {
        if (mensaje) {
            MSG_Enviar("\r\n [SPI FLASH ERROR] [%s] Timeout esperando desbloqueo global.\r\n", TestUart_1);
        }
        return;
    }
    // =========================================================================

    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);
    osDelay(10);

    bool configFail = false;
    const char *failed_part_name = "";

    for (int i = 0; i < PARTITION_COUNT; i++) {
        flash->partitions[i].is_configured = false;
    }

    if (!configFail && !Flash_Config_Partition_Init(flash, PART_CONFIG, 0, 15)) {
        configFail = true;
        failed_part_name = "PART_CONFIG (Sectores 0 al 15)";
    }

    if (!configFail && !Flash_Config_Partition_Init(flash, PART_LOGS, 16, 215)) {
        configFail = true;
        failed_part_name = "PART_LOGS (Sectores 16 al 215)";
    }

    if (!configFail && !Flash_Config_Partition_Init(flash, PART_DATA, 216, 4095)) {
        configFail = true;
        failed_part_name = "PART_DATA (Sectores 216 al 4095)";
    }

    if (mensaje)
    {
        if (!configFail)
        {
            // Bloque único con toda la información de inicialización sin delays excesivos en consola
            snprintf(msn_buffer, sizeof(msn_buffer),
                     "\r\n========================================\r\n"
                     " [SPI FLASH] [%s] INICIALIZACION COMPLETADA\r\n"
                     "========================================\r\n"
                     " ├─ Interfaz SPI   : %p\r\n"
                     " ├─ Puerto CS      : %p (Pin: 0x%04X)\r\n"
                     " ├─ Capacidad      : %lu MB (%lu Bytes)\r\n"
                     " ├─ Size Pagina  : %d Bytes\r\n"
                     " ├─ Total Sectores : %d\r\n"
                     " └─ Mapa:\r\n",
                     __func__,
                     (void*)flash->hspi->Instance,
                     (void*)flash->cs_port,
                     flash->cs_pin,
                     (unsigned long)(FLASH_CAPACITY / 1048576),
                     (unsigned long)FLASH_CAPACITY,
                     FLASH_PAGE_SIZE,
                     FLASH_SECTOR_COUNT);

            MSG_Enviar(msn_buffer, TestUart_1);

            // Bucle de particiones optimizado
            for (int i = 0; i < PARTITION_COUNT; i++)
            {
                if (flash->partitions[i].is_configured)
                {
                    uint16_t start = flash->partitions[i].start_sector;
                    uint16_t count = flash->partitions[i].sector_count;
                    uint16_t end   = start + count - 1;

                    snprintf(msn_buffer, sizeof(msn_buffer),
                             "    ├─ Part. [%d]: %4d Sect. | S: %4d -> %4d\r\n",
                             i, count, start, end);
                    MSG_Enviar(msn_buffer, TestUart_1);
                }
            }

            // Cierre visual
            snprintf(msn_buffer, sizeof(msn_buffer),
                     "    └─ Fin del Mapa de Memoria\r\n"
                     "========================================\r\n\r\n");
            MSG_Enviar(msn_buffer, TestUart_1);
        }
        else
        {
            // Mensaje de error crítico detallando la función exacta
            snprintf(msn_buffer, sizeof(msn_buffer),
                     "\r\n========================================\r\n"
                     " [SPI FLASH ERROR] [%s] ERROR CRITICO\r\n"
                     "========================================\r\n"
                     " ├─ Fallo al mapear    : %s\r\n"
                     " └─ Motivo             : Solapamiento o exceso de limites.\r\n"
                     "========================================\r\n\r\n",
                     __func__,
                     failed_part_name);

            MSG_Enviar(msn_buffer, TestUart_1);
        }
    }
}

bool Flash_Config_Partition_Init(SPI_Flash_TypeDef *flash, Flash_Partition_ID part_id, uint16_t start_sector, uint16_t end_sector)
{
    char msn_buffer[256];

    if (flash == NULL) {
        snprintf(msn_buffer, sizeof(msn_buffer),
                 "\r\n [SPI FLASH ERROR] [%s] Puntero nulo detectado en el objeto flash.\r\n", __func__);
        MSG_Enviar(msn_buffer, TestUart_1);
        return false;
    }

    if (part_id >= PARTITION_COUNT) {
        snprintf(msn_buffer, sizeof(msn_buffer),
                 "\r\n [SPI FLASH ERROR] [%s] ID de particion invalido o fuera de rango (ID: %d).\r\n", __func__, part_id);
        MSG_Enviar(msn_buffer, TestUart_1);
        return false;
    }

    if (end_sector < start_sector) {
        snprintf(msn_buffer, sizeof(msn_buffer),
                 "\r\n [SPI FLASH ERROR] [%s] Rango ilogico en Part. [%d] (Inicio: %d > Fin: %d).\r\n", __func__, part_id, start_sector, end_sector);
        MSG_Enviar(msn_buffer, TestUart_1);
        return false;
    }

    if (end_sector >= FLASH_SECTOR_COUNT) {
        snprintf(msn_buffer, sizeof(msn_buffer),
                 "\r\n [SPI FLASH ERROR] [%s] La Part. [%d] excede el limite fisico (Fin: %d / Max: %d).\r\n", __func__, part_id, end_sector, FLASH_SECTOR_COUNT);
        MSG_Enviar(msn_buffer, TestUart_1);
        return false;
    }

    if (!Flash_Check_Partition_Overlap(flash, part_id, start_sector, end_sector)) {
        return false;
    }

    // 3. CONFIGURACIÓN DE LA PARTICIÓN
    Flash_Partition_TypeDef *part = &flash->partitions[part_id];

    memset(part, 0, sizeof(Flash_Partition_TypeDef));

    part->id             = part_id;
    part->start_sector   = start_sector;
    part->end_sector     = end_sector;
    part->sector_count   = (end_sector - start_sector) + 1;
    part->current_sector = start_sector;
    part->sector_offset  = 0;
    part->is_configured  = true;

    return true;
}

bool Flash_Check_Partition_Overlap(SPI_Flash_TypeDef *flash, Flash_Partition_ID part_id, uint16_t start_sector, uint16_t end_sector)
{
    char msn_buffer[256];

    for (int i = 0; i < PARTITION_COUNT; i++)
    {
        if (i != part_id && flash->partitions[i].is_configured)
        {
            uint16_t other_start = flash->partitions[i].start_sector;
            uint16_t other_end   = flash->partitions[i].end_sector;

            if ((start_sector <= other_end) && (other_start <= end_sector))
            {
                snprintf(msn_buffer, sizeof(msn_buffer),
                         "\r\n [SPI FLASH ERROR CRITICO] [%s] Solapamiento detectado:\r\n"
                         " ├─ Particion intentada : [%d] (Sectores: %d -> %d)\r\n"
                         " └─ Choca con Part.     : [%d] (Sectores: %d -> %d)\r\n",
                         __func__,
                         part_id, start_sector, end_sector,
                         i, other_start, other_end);

                MSG_Enviar(msn_buffer, TestUart_1);
                return false;
            }
        }
    }
    return true;
}

bool Flash_detect_chip_Init(SPI_Flash_TypeDef *flash)
{
    if (flash == NULL) {
        return false;
    }

    uint8_t cmd = FLASH_CMD_ID;
    uint8_t id_rx[3] = {0};
    char msn_buffer[256];

    // 1. Adquirir Mutex con control de error y __func__
    if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
        snprintf(msn_buffer, sizeof(msn_buffer),
                 "\r\n [SPI FLASH ERROR] [%s] Timeout de Mutex: El bus SPI esta ocupado.\r\n", __func__);
        MSG_Enviar(msn_buffer, TestUart_1);
        return false;
    }

    // 2. Transacción física SPI protegida
    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef tx_status = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
    HAL_StatusTypeDef rx_status = HAL_SPI_Receive(flash->hspi, id_rx, 3, WRITE_READ_NORMAL_VELOCITY);
    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

    osMutexRelease(flash->spi_mutex);

    char fabricante[32] = "Desconocido";
    char capacidad[32] = "Desconocida";

    // 3. Comprobación de estado HAL
    if (tx_status != HAL_OK || rx_status != HAL_OK) {
        snprintf(msn_buffer, sizeof(msn_buffer),
                 "\r\n [SPI FLASH ERROR] [%s] HAL_SPI ha devuelto un error (Timeout o Busy).\r\n", __func__);
        MSG_Enviar(msn_buffer, TestUart_1);
        return false;
    }

    // 4. Comprobación de integridad del ID (0x00 o 0xFF)
    if (id_rx[0] == 0x00 || id_rx[0] == 0xFF) {
        if (id_rx[0] == 0x00) {
            snprintf(msn_buffer, sizeof(msn_buffer),
                     "\r\n [SPI FLASH ERROR] [%s] Fallo comunicacion SPI.\r\n"
                     " └─ ID: 0x00 (Posible corto a GND o chip dañado).\r\n\r\n", __func__);
            MSG_Enviar(msn_buffer, TestUart_1);
            return false;
        }

        if (id_rx[0] == 0xFF) {
            snprintf(msn_buffer, sizeof(msn_buffer),
                     "\r\n [SPI FLASH ERROR] [%s] Fallo comunicacion SPI.\r\n"
                     " └─ ID: 0xFF (Chip ausente o pines desoldados).\r\n\r\n", __func__);
            MSG_Enviar(msn_buffer, TestUart_1);
            return false;
        }
    }

    // 5. Identificación del Fabricante
    if (id_rx[0] == 0xC8) {
        strcpy(fabricante, "GigaDevice (GD25Q...)");
    }
    else if (id_rx[0] == 0xEF) {
        strcpy(fabricante, "Winbond (W25Q...)");
    }
    else if (id_rx[0] == 0xC2) {
        strcpy(fabricante, "Macronix (MX25L...)");
    }

    // 6. Identificación de la Capacidad
    if (id_rx[2] == 0x18) {
        strcpy(capacidad, "16 MB (128 Mbit)");
    }
    else if (id_rx[2] == 0x17) {
        strcpy(capacidad, "8 MB (64 Mbit)");
    }

    // 7. Envío del reporte de hardware unificado y limpio (sin delays)
    snprintf(msn_buffer, sizeof(msn_buffer),
             "\r\n [SPI FLASH] [%s] Analisis de Hardware Exitoso:\r\n"
             " ├─ JEDEC ID Raw : %02X %02X %02X\r\n"
             " ├─ Fabricante   : %s\r\n"
             " └─ Capacidad    : %s\r\n",
             __func__,
             id_rx[0], id_rx[1], id_rx[2],
             fabricante,
             capacidad);

    if (!MSG_Enviar_Longitud_Limit(msn_buffer)) {
        MSG_Enviar(msn_buffer, TestUart_1);
    }

    return true;
}

