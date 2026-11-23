#include <string.h>
#include <stdio.h>

#include "cmsis_os.h"

#include "SPI_Flash_Memory/SPI_Flash_Memory.h"
#include "MsgUART/TxUart/TxUart.h"


extern I_TestUart Uart1;



bool Flash_Write_All_Sectors_Data(SPI_Flash_TypeDef *flash, uint32_t total_sectors, uint32_t pages_per_sector, uint8_t data_value)
{
    if (flash == NULL) {
        return false;
    }

    osDelay(TXFLASHLOG);
    MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH] [Flash_Write_All] Iniciando escritura masiva por páginas...\r\n", Uart1);
    osDelay(TXFLASHLOG);

    // =========================================================================
    // DESBLOQUEO GLOBAL DE HARDWARE (Asegurar permisos de escritura en toda la Flash)
    // =========================================================================
    if (!Flash_Complement_Write_Enable(flash)) {
        MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH ERROR] Fallo en Write Enable para Desbloqueo.\r\n", Uart1);
        return false;
    }

    osDelay(TXFLASHLOG);

    uint8_t cmd_unprotect = 0x01;
    uint8_t status_val[2] = {0x00, 0x00};

    if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
        MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH ERROR] Timeout de Mutex en desbloqueo.\r\n", Uart1);
        return false;
    }

    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(flash->hspi, &cmd_unprotect, 1, WRITE_READ_NORMAL_VELOCITY);
    HAL_SPI_Transmit(flash->hspi, status_val, 2, WRITE_READ_NORMAL_VELOCITY);
    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

    osMutexRelease(flash->spi_mutex);

    if (!Flash_Comeplement_Wait_For_Ready(flash, ERASE_ONE_SECTOR)) {
        MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH ERROR] Timeout esperando desbloqueo global.\r\n", Uart1);
        return false;
    }
    // =========================================================================

    // Búfer unificado: 4 bytes de comando/dirección + 256 bytes de datos de la página
    uint8_t packet_buffer[260];

    // Rellenamos la parte de datos con el valor proporcionado
    memset(&packet_buffer[4], data_value, 256);

    char buf_progreso[256];

    // =========================================================================
    // BUCLE DE BARRIDO POR SECTORES Y PÁGINAS
    // =========================================================================
    for (uint32_t sector = 0; sector < total_sectors; ++sector) {
        for (uint32_t page = 0; page < pages_per_sector; ++page) {

            // 1. Habilitar la escritura obligatoriamente antes de CADA página
            if (!Flash_Complement_Write_Enable(flash)) {
                osDelay(TXFLASHLOG);
                MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH ERROR] Error en Write Enable por página.\r\n", Uart1);
                osDelay(TXFLASHLOG);
                return false;
            }

            // 2. Calcular la dirección física absoluta
            uint32_t absolute_address = (sector * FLASH_SECTION_SIZE) + (page * FLASH_PAGE_SIZE);

            // 3. Construir la cabecera directamente al inicio del paquete unificado
            packet_buffer[0] = 0x02; // Comando Page Program estándar
            packet_buffer[1] = (uint8_t)((absolute_address >> 16) & 0xFF);
            packet_buffer[2] = (uint8_t)((absolute_address >> 8) & 0xFF);
            packet_buffer[3] = (uint8_t)(absolute_address & 0xFF);

            // 4. Proteger el bus SPI con Mutex de FreeRTOS
            if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {


                osDelay(TXFLASHLOG);
                MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH ERROR] Timeout de Mutex en escritura de página.\r\n", Uart1);
                osDelay(TXFLASHLOG);
                return false;
            }

            // 5. Transmitir  el paquete de golpe (Comando + Dirección + 256 bytes) en una sola operación continua
            HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
            HAL_StatusTypeDef estado_spi = HAL_SPI_Transmit(flash->hspi, packet_buffer, 260, WRITE_READ_NORMAL_VELOCITY);
            HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

            osMutexRelease(flash->spi_mutex);
            osDelay(200);

            if (estado_spi != HAL_OK) {
                osDelay(TXFLASHLOG);
                MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH ERROR] Fallo de hardware SPI transmitiendo el paquete unificado.\r\n", Uart1);
                osDelay(TXFLASHLOG);
                return false;
            }

            // 6. Esperar a que el chip termine de programar la página internamente (control por bit BSY)
            if (!Flash_Comeplement_Wait_For_Ready(flash, WRITE_READ_NORMAL_VELOCITY)) {
                osDelay(TXFLASHLOG);
                MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH ERROR] Timeout esperando a que la página se escriba.\r\n", Uart1);
                osDelay(TXFLASHLOG);
                return false;
            }

            osDelay(200);

            // =====================================================================
            // REPORTE EN TIEMPO REAL DE CADA PÁGINA ESCRITA
            // =====================================================================
            snprintf(buf_progreso, sizeof(buf_progreso),
                     " [SPI FLASH] Escribiendo... Sector %lu / %lu | Pág %lu / %lu (Dato: 0x%02X)\r\n",
                     (unsigned long)sector,
                     (unsigned long)total_sectors,
                     (unsigned long)page,
                     (unsigned long)pages_per_sector,
                     data_value);

            osDelay(TXFLASHLOG);
            MSG_Enviar(MSG_GENERAL,buf_progreso, Uart1);
            osDelay(TXFLASHLOG);
            // =====================================================================
        }
    }
    // =========================================================================

    osDelay(TXFLASHLOG);
    MSG_Enviar(MSG_GENERAL," [SPI FLASH] [Flash_Write_All] Escritura masiva completada con exito.\r\n", Uart1);
    osDelay(TXFLASHLOG);

    // =========================================================================
    // DIAGNÓSTICO DE MEMORIA LIBRE (High Water Mark de la tarea actual)
    // =========================================================================
    {
        char *nombre_tarea = (char*)pcTaskGetName(NULL);
        UBaseType_t libres_words = uxTaskGetStackHighWaterMark(NULL);
        uint32_t libres_bytes = (uint32_t)(libres_words * 4);

        char buf_memoria[128];
        snprintf(buf_memoria, sizeof(buf_memoria),
                 "\r\n [DIAGNOSTICO] [Flash_Write_All]\r\n"
                 "  |- Tarea Activa : %s\r\n"
                 "  \\- Margen Libre : %lu bytes\r\n",
                 nombre_tarea,
                 (unsigned long)libres_bytes);

        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,buf_memoria, Uart1);
        osDelay(TXFLASHLOG);
    }
    // =========================================================================

    return true;
};


bool Flash_Write_One_Sector_Partition(SPI_Flash_TypeDef *flash, Flash_Partition_ID partition, uint32_t addr, uint32_t dataLenght , const uint8_t *data)
{
    if (flash == NULL) {
        return false;
    }

    // Comprobaciones  -->
    if (!Flash_Complement_Is_Sector_partition(flash, partition, addr)) {
        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH] [Flash_Write_One_Sector] No estar particionado\r\n", Uart1);
        osDelay(TXFLASHLOG);
        return false;
    }

    if (Flash_Complement_HaveData_Sector(flash, partition, addr)) {
    	if(!Flash_Erase_One_Hex_For_Sector(flash, addr)){
    		osDelay(TXFLASHLOG);
			MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH] [Flash_Write_One_Sector_Partition] Fallo borrado del sector con datos\r\n", Uart1);
			osDelay(TXFLASHLOG);
			return false;
    	}
    }

    if (dataLenght == 0 || dataLenght > FLASH_SECTION_SIZE || data == NULL) {
        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH] [Flash_Erase_One_Sector] Parámetros inválidos-vacíos-limite, no se realiza borrado \r\n", Uart1);
        osDelay(TXFLASHLOG);
        return false;
    }

    osDelay(TXFLASHLOG);
    MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH] [Flash_Write_One_Sector] Iniciando escritura masiva por páginas...\r\n", Uart1);
    osDelay(TXFLASHLOG);


    if (!Flash_Complement_Write_Enable(flash)) {
    	osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH ERROR] Fallo en Write Enable para Desbloqueo.\r\n", Uart1);
        osDelay(TXFLASHLOG);
        return false;
    }


    // Desconexion -->
    uint8_t cmd_unprotect = FLASH_CMD_BP;
    uint8_t status_val[2] = {0x00, 0x00};

    if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
        MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH ERROR] Timeout de Mutex en desbloqueo.\r\n", Uart1);
        return false;
    }

    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(flash->hspi, &cmd_unprotect, 1, WRITE_READ_NORMAL_VELOCITY);
    HAL_SPI_Transmit(flash->hspi, status_val, 2, WRITE_READ_NORMAL_VELOCITY);
    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

    osMutexRelease(flash->spi_mutex);

    if (!Flash_Comeplement_Wait_For_Ready(flash, ERASE_ONE_SECTOR)) {
        MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH ERROR] Timeout esperando desbloqueo global.\r\n", Uart1);
        return false;
    }

    uint8_t packet_buffer[260];
    char buf_progreso[80];
    uint32_t pages_per_sector = 4096 / 256;

    //Escritura -->
    for (uint32_t page = 0; page < pages_per_sector; ++page) {


        uint32_t offset = page * 256;

        if (offset >= dataLenght) {
            break;
        }

        uint32_t bytes_to_copy = 256;
        if (offset + 256 > dataLenght) {
            bytes_to_copy = dataLenght - offset;
        }


        memset(packet_buffer, 0xFF, sizeof(packet_buffer));
        memcpy(&packet_buffer[4], &data[offset], bytes_to_copy);


        if (!Flash_Complement_Write_Enable(flash)) {
            osDelay(TXFLASHLOG);
            MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH ERROR] Error en Write Enable por página.\r\n", Uart1);
            osDelay(TXFLASHLOG);
            return false;
        }


        uint32_t absolute_address = addr + offset;


        packet_buffer[0] = FLASH_CMD_PAGE_PROGRAM;
        packet_buffer[1] = (uint8_t)((absolute_address >> 16) & 0xFF);
        packet_buffer[2] = (uint8_t)((absolute_address >> 8) & 0xFF);
        packet_buffer[3] = (uint8_t)(absolute_address & 0xFF);


        if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
            osDelay(TXFLASHLOG);
            MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH ERROR] Timeout de Mutex en escritura de página.\r\n", Uart1);
            osDelay(TXFLASHLOG);
            return false;
        }


        HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
        HAL_StatusTypeDef estado_spi = HAL_SPI_Transmit(flash->hspi, packet_buffer, 260, HAL_MAX_DELAY);
        HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

        osMutexRelease(flash->spi_mutex);


        if (estado_spi != HAL_OK) {
            osDelay(TXFLASHLOG);
            MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH ERROR] Fallo de hardware SPI transmitiendo el paquete unificado.\r\n", Uart1);
            osDelay(TXFLASHLOG);
            return false;
        }


        if (!Flash_Comeplement_Wait_For_Ready(flash, WRITE_READ_NORMAL_VELOCITY)) {
            osDelay(TXFLASHLOG);
            MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH ERROR] Timeout esperando a que la página se escriba.\r\n", Uart1);
            osDelay(TXFLASHLOG);
            return false;
        }



        snprintf(buf_progreso, sizeof(buf_progreso),
                 " [SPI FLASH] Escribiendo... Sector %lu | Pág %lu \r\n",
                 (unsigned long)addr,
                 (unsigned long)page);

        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,buf_progreso, Uart1);
        osDelay(TXFLASHLOG);
    }


    osDelay(TXFLASHLOG);
    MSG_Enviar(MSG_GENERAL," [SPI FLASH] [Flash_Write_One_Sector] Escritura de sector completada con exito.\r\n", Uart1);
    osDelay(TXFLASHLOG);


    {
        char *nombre_tarea = (char*)pcTaskGetName(NULL);
        UBaseType_t libres_words = uxTaskGetStackHighWaterMark(NULL);
        uint32_t libres_bytes = (uint32_t)(libres_words * 4);

        char buf_memoria[128];
        snprintf(buf_memoria, sizeof(buf_memoria),
                 "\r\n [DIAGNOSTICO] [Flash_Write_One_Sector]\r\n"
                 "  |- Tarea Activa : %s\r\n"
                 "  \\- Margen Libre : %lu bytes\r\n",
                 nombre_tarea,
                 (unsigned long)libres_bytes);

        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,buf_memoria, Uart1);
        osDelay(TXFLASHLOG);
    }


    return true;
}
