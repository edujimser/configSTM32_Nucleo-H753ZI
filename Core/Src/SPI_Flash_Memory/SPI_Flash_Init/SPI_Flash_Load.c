#include <string.h>
#include <stdio.h>

#include "cmsis_os.h"

#include "SPI_Flash_Memory/SPI_Flash_Memory.h"
#include "MsgUART/TxUart/TxUart.h"

extern I_TestUart Uart1;

/*
 * ==========================================================================================
 *                             DIAGRAMA DE FLUJO: Flash_Load_Init()
 * ==========================================================================================
 *
 *                                 ┌────────────────────────┐
 *                                 │ INICIO: Flash_Load_Init│
 *                                 └───────────┬────────────┘
 *                                             ▼
 *                            [1]  ¿Puntero flash válido? (flash != NULL)
 *                                 ├── NO ──► [ ABORTAR (return false) ]
 *                                 ▼ SÍ
 *                                 ┌────────────────────────┐
 *                                 │ Bloquear partición     │
 *                                 │ (load=false, init=false│
 *                                 └───────────┬────────────┘
 *                                             ▼
 *                            [2]  ¿MBR Válido y Coincide? (Flash_Load_Check)
 *                                 │ (Comprueba clave, particiones y Checksum)
 *                                 │
 *                                 ├── SÍ ──► [ SISTEMA LISTO (return true) ]
 *                                 │
 *                                 ▼ NO (MBR Corrupto/Inválido o Cambiado)
 *                                 ┌────────────────────────┐
 *                                 │ Iniciar Formateo Total │
 *                                 └───────────┬────────────┘
 *                                             ▼
 *                            [3]  ¿Borrado Completo del Chip Exitoso?
 *                                 │ (Flash_Load_Erase_All_Sectors)
 *                                 ├── NO ──► [ ERROR BORRADO (return false) ]
 *                                 ▼ SÍ
 *                                 ┌────────────────────────┐
 *                                 │ Generar nuevo MBR en   │
 *                                 │ RAM (Clave, Checksum)  │
 *                                 └───────────┬────────────┘
 *                                             ▼
 *                            [4]  ¿Escritura del nuevo MBR Exitosa?
 *                                 │ (Flash_Load_Write_Sector)
 *                                 ├── NO ──► [ ERROR ESCRITURA (return false) ]
 *                                 ▼ SÍ
 *                                 ┌────────────────────────┐
 *                                 │ Verificar Integridad   │
 *                                 └───────────┬────────────┘
 *                                             ▼
 *                            [5]  ¿Segunda validación MBR Exitosa?
 *                                 │ (Flash_Load_Check)
 *                                 ├── NO ──► [ ERROR FATAL HW (return false) ]
 *                                 │
 *                                 ▼ SÍ
 *                                 [ SISTEMA RECUPERADO (return true) ]
 *                                             │
 *                                             ▼
 *                                 ┌────────────────────────┐
 *                                 │    FIN DEL PROCESO     │
 *                                 └────────────────────────┘
 * ==========================================================================================
 */


bool Flash_Load_Init(SPI_Flash_TypeDef *flash)
{
    if (flash == NULL) {
        return false;
    }

    // =========================================================================
	// DATA CONTROL
	// =========================================================================

	flash->load = false;
	flash->partitions[FLASH_SECTION_LOAD].init = false;
    flash->partitions[FLASH_SECTION_LOAD].run = false;


    osDelay(TXFLASHLOG);
       MSG_Enviar(MSG_GENERAL,
    	   "\r\n==========================================================\r\n"
    	   " [SPI FLASH] [Flash_Load_Init] INIT LOAD\r\n"
    	   "==========================================================\r\n",
    	   Uart1
       );
    osDelay(TXFLASHLOG);

    // =========================================================================
    // MIRAMOS SI LOS DATOS ESTÁN CORRECTOS
    // =========================================================================
    bool status = Flash_Load_Check(flash);

    if (status) {
    	flash->load = true;
    	flash->partitions[FLASH_SECTION_LOAD].init = true;
        flash->partitions[FLASH_SECTION_LOAD].run = true;

        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL," [SPI FLASH] [Load_Status] MBR OK. Sistema listo.\r\n", Uart1);
        osDelay(TXFLASHLOG);

        osDelay(TXFLASHLOG);
		  MSG_Enviar(MSG_GENERAL,
		   "\r\n==========================================================\r\n"
		   " [SPI FLASH] [Flash_Load_Init] FINAL LOAD\r\n"
		   "==========================================================\r\n",
		   Uart1
		  );
	   osDelay(TXFLASHLOG);
        return true;
    }

    // =========================================================================
	// DATOS INCORRECTO
	// =========================================================================

    osDelay(TXFLASHLOG);
    MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH WARNING] [Load_Status] MBR invalido o corrupto. Iniciando formateo...\r\n", Uart1);
    osDelay(TXFLASHLOG);



    Master_Boot_Record_TypeDef mbr;
    memset(&mbr, 0, sizeof(Master_Boot_Record_TypeDef));

    if (!Flash_Load_Erase_All_Sectors(flash)) {
    	flash->load = false;
		flash->partitions[FLASH_SECTION_LOAD].init = true;
		flash->partitions[FLASH_SECTION_LOAD].run = false;

        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH ERROR] [Load_Status] Fallo al borrar todos los sectores.\r\n", Uart1);
        osDelay(TXFLASHLOG);

        osDelay(TXFLASHLOG);
		  MSG_Enviar(MSG_GENERAL,
		   "\r\n==========================================================\r\n"
		   " [SPI FLASH] [Flash_Load_Init] FINAL LOAD\r\n"
		   "==========================================================\r\n",
		   Uart1
		  );
	   osDelay(TXFLASHLOG);

        return false;
    }

    mbr.format_key = FLASH_SECRET_NUMER;
    memcpy(mbr.partitions, flash->partitions, sizeof(flash->partitions));
    mbr.checksum = Flash_Calculate_Check_MBR_Checksum(&mbr);

    if (!Flash_Load_Write_Sector(flash, flash->partitions[FLASH_SECTION_LOAD].start_sector, (uint8_t*)&mbr, sizeof(Master_Boot_Record_TypeDef))) {

      	flash->load = false;
		flash->partitions[FLASH_SECTION_LOAD].init = true;
		flash->partitions[FLASH_SECTION_LOAD].run = false;

        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH ERROR] [Load_Status] Fallo al escribir nuevo MBR.\r\n", Uart1);
        osDelay(TXFLASHLOG);
        return false;
    }

    osDelay(TXFLASHLOG);
    MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH] [Load_Status] Formateo completado. Verificando integridad...\r\n", Uart1);
    osDelay(TXFLASHLOG);

    status = Flash_Load_Check(flash);

    if (status) {
      	flash->load = true;
		flash->partitions[FLASH_SECTION_LOAD].init = true;
		flash->partitions[FLASH_SECTION_LOAD].run = true;

        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL," [SPI FLASH] [Load_Status] Verificacion MBR exitosa. Sistema recuperado.\r\n", Uart1);
        osDelay(TXFLASHLOG);
        return true;
    }

    osDelay(TXFLASHLOG);
    MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH FATAL ERROR] [Load_Status] Imposible recuperar MBR. Memoria dañada.\r\n", Uart1);
    osDelay(TXFLASHLOG);


    return false;
}

bool Flash_Load_Read_Data(SPI_Flash_TypeDef *flash, uint32_t absolute_address, uint8_t *buffer, uint32_t length)
{
    char buf_msg[128];

    if (flash == NULL || buffer == NULL || length == 0) {
        return false;
    }

    osDelay(TXFLASHLOG);
    snprintf(buf_msg, sizeof(buf_msg), "\r\n [SPI FLASH] [Read_Data] Leyendo %lu bytes desde 0x%06lX...\r\n",
             (unsigned long)length, (unsigned long)absolute_address);
    MSG_Enviar(MSG_GENERAL,buf_msg, Uart1);
    osDelay(TXFLASHLOG);

    uint8_t tx_cmd[4];
    uint8_t rx_dummy[4];

    tx_cmd[0] = FLASH_CMD_READ;
    tx_cmd[1] = (uint8_t)((absolute_address >> 16) & 0xFF);
    tx_cmd[2] = (uint8_t)((absolute_address >> 8) & 0xFF);
    tx_cmd[3] = (uint8_t)(absolute_address & 0xFF);

    if (osMutexAcquire(flash->spi_mutex, pdMS_TO_TICKS(1000)) != osOK) {
        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH ERROR] [Read_Data] Timeout de Mutex SPI.\r\n", Uart1);
        osDelay(TXFLASHLOG);
        return false;
    }

    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);

    HAL_StatusTypeDef status = HAL_SPI_TransmitReceive(flash->hspi, tx_cmd, rx_dummy, 4, WRITE_READ_NORMAL_VELOCITY);

    if (status == HAL_OK) {
        status = HAL_SPI_Receive(flash->hspi, buffer, length, WRITE_READ_NORMAL_VELOCITY);
    }

    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);
    osMutexRelease(flash->spi_mutex);

    if (status != HAL_OK) {
        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH ERROR] [Read_Data] Fallo en TransmitReceive/Receive.\r\n", Uart1);
        osDelay(TXFLASHLOG);
        return false;
    }

    if(FLASH_DEBUG_THREAD_SIZE){
		for (uint32_t i = 0; i < length; i += 16) {
				char hex_line[64] = {0};
				int offset = 0;

				for (uint32_t j = 0; (j < 16) && ((i + j) < length); j++) {
					offset += snprintf(hex_line + offset, sizeof(hex_line) - offset, "%02X ", buffer[i + j]);
				}

				snprintf(buf_msg, sizeof(buf_msg), "  [+%03lX] %s\r\n", (unsigned long)i, hex_line);
				osDelay(TXFLASHLOG);
				MSG_Enviar(MSG_GENERAL,buf_msg, Uart1);
				osDelay(TXFLASHLOG);
		}
    }

	osDelay(TXFLASHLOG);
	MSG_Enviar(MSG_GENERAL," [SPI FLASH] [Read_Data] Lectura completada con exito.\r\n", Uart1);
	osDelay(TXFLASHLOG);

    Flash_Complement_Data_Thread(FLASH_DEBUG_THREAD_SIZE);

    return true;
}

bool Flash_Load_Check(SPI_Flash_TypeDef *flash)
{


    if (flash == NULL) {
        return false;
    }

    osDelay(TXFLASHLOG);
    MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH] [Flash_Load_Check] Iniciando validacion MBR...\r\n", Uart1);
    osDelay(TXFLASHLOG);

    Master_Boot_Record_TypeDef mbr;
    memset(&mbr, 0, sizeof(Master_Boot_Record_TypeDef));

    if (!Flash_Load_Read_Data(flash, flash->partitions[FLASH_SECTION_LOAD].start_sector, (uint8_t*)&mbr, sizeof(Master_Boot_Record_TypeDef))) {
        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH ERROR] [Flash_Load_Check] Fallo lectura SPI.\r\n", Uart1);
        osDelay(TXFLASHLOG);
        return false;
    }

    char buf_msg[256];

    if (mbr.format_key != FLASH_SECRET_NUMER) {

    	if(FLASH_DEBUG_THREAD_SIZE) {


			snprintf(buf_msg, sizeof(buf_msg),
					 "\r\n [SPI FLASH ERROR] Clave incorrecta.\r\n"
					 "  |- RAM (Esperado) : 0x%08lX\r\n"
					 "  \\- FLASH (Leido)  : 0x%08lX\r\n",
					 (unsigned long)FLASH_SECRET_NUMER, (unsigned long)mbr.format_key);
			osDelay(TXFLASHLOG);
			MSG_Enviar(MSG_GENERAL,buf_msg, Uart1);
			osDelay(TXFLASHLOG);
			return false;
    	}
    }

    for (int i = 0; i < FLASH_SECTION_COUNT_END; ++i) {
        if (flash->partitions[i].id != mbr.partitions[i].id ||
            flash->partitions[i].start_sector != mbr.partitions[i].start_sector ||
            flash->partitions[i].end_sector != mbr.partitions[i].end_sector ||
            flash->partitions[i].num_sector_count != mbr.partitions[i].num_sector_count)
        {
        	if(FLASH_DEBUG_THREAD_SIZE) {
				snprintf(buf_msg, sizeof(buf_msg),
						 "\r\n [SPI FLASH ERROR] Discrepancia en Particion %d\r\n"
						 "  |- RAM   -> ID: %u | Start: %u | End: %u | Count: %u\r\n"
						 "  \\- FLASH -> ID: %u | Start: %u | End: %u | Count: %u\r\n",
						 i,
						 flash->partitions[i].id, flash->partitions[i].start_sector,
						 flash->partitions[i].end_sector, flash->partitions[i].num_sector_count,
						 mbr.partitions[i].id, mbr.partitions[i].start_sector,
						 mbr.partitions[i].end_sector, mbr.partitions[i].num_sector_count);

				osDelay(TXFLASHLOG);
				MSG_Enviar(MSG_GENERAL,buf_msg, Uart1);
				osDelay(TXFLASHLOG);
        	}
            return false;
        }
    }

    if (!Flash_Load_Check_MBR_Checksum(&mbr)) {
        return false;
    }


    osDelay(TXFLASHLOG);
    MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH] [Config_Check] Validacion MBR exitosa. Todo coincide.\r\n", Uart1);
    osDelay(TXFLASHLOG);

    Flash_Complement_Data_Thread(FLASH_DEBUG_THREAD_SIZE);

    return true;
}

uint16_t Flash_Calculate_Check_MBR_Checksum(Master_Boot_Record_TypeDef *mbr) {
    uint16_t calc_checksum = 0;
    uint8_t *ptr = (uint8_t*)mbr;

    // Obtiene los bytes exactos desde el inicio hasta antes del checksum
    uint32_t size = offsetof(Master_Boot_Record_TypeDef, checksum);

    for (uint32_t i = 0; i < size; ++i) {
        calc_checksum ^= ptr[i];
    }

    return calc_checksum;
}

bool Flash_Load_Check_MBR_Checksum(Master_Boot_Record_TypeDef *mbr) {
    // Reutilizamos la misma función de cálculo para garantizar que
    // el método sea 100% idéntico y evitar duplicar código.
    uint16_t calc_checksum = Flash_Calculate_Check_MBR_Checksum(mbr);

    if (calc_checksum != mbr->checksum) {
    	if(FLASH_DEBUG_THREAD_SIZE){
			char buf_msg[128];
			snprintf(buf_msg, sizeof(buf_msg),
					 "\r\n [SPI FLASH ERROR] Checksum del MBR corrupto.\r\n"
					 "  |- Calculado : 0x%04X\r\n"
					 "  \\- Leido     : 0x%04X\r\n",
					 calc_checksum, mbr->checksum);

			osDelay(TXFLASHLOG);
			MSG_Enviar(MSG_GENERAL,buf_msg, Uart1);
			osDelay(TXFLASHLOG);
    	}
        return false;
    }

    return true;
}

bool Flash_Load_Erase_All_Sectors(SPI_Flash_TypeDef *flash){
	if (flash == NULL) {
		return false;
	}

	if (!flash->init){
		return false;
	}

	HAL_StatusTypeDef estado_spi;
	HAL_StatusTypeDef estado_spi_2;
	uint8_t cmd = 0;

	osDelay(TXFLASHLOG);
			MSG_Enviar(MSG_GENERAL,"\r\n Iniciando borrado masivo por fallo de configuracion\r\n", Uart1);
	osDelay(TXFLASHLOG);

	// =========================================================================
	// DIAGNÓSTICO INICIAL (Lectura del estado en frío del componente)
	// =========================================================================
	osDelay(TXFLASHLOG);
	MSG_Enviar(MSG_GENERAL,"\r\n --- DIAGNOSTICO PRE-BORRADO ---\r\n [1] Estado de la Flash en frio:\r\n", Uart1);
	osDelay(TXFLASHLOG);

	Flash_Read_Component(flash);

	osDelay(TXFLASHLOG);
	MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH] [Flash_Erase_All] Preparando secuencia de borrado...\r\n", Uart1);
	osDelay(TXFLASHLOG);

	// =========================================================================
	// BORRADO
	// =========================================================================

	if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
		MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH FATAL] No se pudo secuestrar el bus SPI.\r\n", Uart1);
		return false;
	}




	cmd = FLASH_CMD_WRITE_ENABLE;

	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	estado_spi = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);


	if (estado_spi != HAL_OK) {
		MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH FATAL] Fallo al enviar comando Write para Erase.\r\n", Uart1);
		osMutexRelease(flash->spi_mutex);
		return false;
	}




	cmd = FLASH_CMD_BP;
	uint8_t status_val[2] = {0x00, 0x00};

	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	estado_spi = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
	estado_spi_2 = HAL_SPI_Transmit(flash->hspi, status_val, 2, WRITE_READ_NORMAL_VELOCITY);
	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

	if (estado_spi != HAL_OK || estado_spi_2 != HAL_OK) {
		MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH FATAL] Fallo al enviar comando eliminar bloqueos zona alta.\r\n", Uart1);
		osMutexRelease(flash->spi_mutex);
		return false;
	}




	cmd = FLASH_CMD_WRITE_ENABLE;

	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	estado_spi = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);


	if (estado_spi != HAL_OK) {
		MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH FATAL] Fallo al enviar comando Write para Erase.\r\n", Uart1);
		osMutexRelease(flash->spi_mutex);
		return false;
	}




	cmd = FLASH_CMD_CHIP_ERASE;

	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	estado_spi = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

	if (estado_spi != HAL_OK) {
		MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH FATAL] Fallo al enviar comando Erase.\r\n", Uart1);
		osMutexRelease(flash->spi_mutex);
		return false;
	}

	osMutexRelease(flash->spi_mutex);

	// =========================================================================
	// COMPROBACION DE TIEMPO
	// =========================================================================

	osDelay(10);
	uint32_t timeUse = 0;
	char buf_msg[256];


	if (!Flash_Wait_Busy(flash, ERASE_ALL_WAIT_BUSY, &timeUse)) {
		MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH FATAL] TIMEOUT: El chip se quedo bloqueado borrando.\r\n", Uart1);
	} else {
		snprintf(buf_msg, sizeof(buf_msg),
				 "\r\n [SPI FLASH OK] Borrado Finalizado. Tiempo: %lu ms. Memoria virgen.\r\n",
				 (unsigned long)timeUse);

		MSG_Enviar(MSG_GENERAL,buf_msg, Uart1);
	}



	osDelay(TXFLASHLOG * 15);

   return true;

}

bool Flash_Load_Write_Sector(SPI_Flash_TypeDef *flash, uint32_t sector, const uint8_t *data, uint32_t dataLenght){
	if (flash == NULL) {
		return false;
	}


	if (dataLenght == 0 || dataLenght > FLASH_SECTION_SIZE || data == NULL) {
		osDelay(TXFLASHLOG);
		MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH] [Flash_Load_Write_Sector] Parámetros inválidos-vacíos-limite, no se realiza borrado \r\n", Uart1);
		osDelay(TXFLASHLOG);
		return false;
	}

	osDelay(TXFLASHLOG);
	MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH] [Flash_Write_One_Sector] Iniciando escritura...\r\n", Uart1);
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
	uint32_t pages_per_sector = FLASH_SECTION_SIZE / FLASH_PAGE_SIZE;

	//Escritura -->
	for (uint32_t page = 0; page < pages_per_sector; ++page) {


		uint32_t offset = page * FLASH_PAGE_SIZE;

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


		uint32_t absolute_address = sector + offset;


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
		HAL_StatusTypeDef estado_spi = HAL_SPI_Transmit(flash->hspi, packet_buffer, 260, WRITE_READ_NORMAL_VELOCITY);
		HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

		osMutexRelease(flash->spi_mutex);
		osDelay(10);

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

		osDelay(10);

		if(FLASH_DEBUG_THREAD_SIZE){

			snprintf(buf_progreso, sizeof(buf_progreso),
					 " [SPI FLASH] Escribiendo... Sector %lu | Pág %lu \r\n",
					 (unsigned long)sector,
					 (unsigned long)page);

			osDelay(TXFLASHLOG);
			MSG_Enviar(MSG_GENERAL,buf_progreso, Uart1);
			osDelay(TXFLASHLOG);
		}
	}


	osDelay(TXFLASHLOG);
	MSG_Enviar(MSG_GENERAL," [SPI FLASH] [Flash_Write_One_Sector] Escritura de sector completada con exito.\r\n", Uart1);
	osDelay(TXFLASHLOG);


	Flash_Complement_Data_Thread(FLASH_DEBUG_THREAD_SIZE);

	return true;
}



