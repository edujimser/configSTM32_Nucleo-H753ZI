#include <string.h>
#include <stdio.h>

#include "cmsis_os.h"

#include "SPI_Flash_Memory/SPI_Flash_Memory.h"
#include "MsgUART/TxUart/TxUart.h"


extern I_TestUart TestUart_1;





bool Flash_Erase_All(SPI_Flash_TypeDef *flash)
{
    if (flash == NULL) {
        return false;
    }


    osDelay(TXFLASHLOG);
    MSG_Enviar("\r\n --- DIAGNOSTICO PRE-BORRADO ---\r\n [1] Estado de la Flash en frio:\r\n", TestUart_1);
    osDelay(TXFLASHLOG);

    Flash_Read_Component(flash);
    osDelay(TXFLASHLOG);


    if (!Flash_Write_Enable(flash)) {
    	osDelay(TXFLASHLOG);
        MSG_Enviar("\r\n [SPI FLASH ERROR] [Flash_Erase_All] Error en Write Enable.\r\n", TestUart_1);
        osDelay(TXFLASHLOG);
        return false;
    }


    osDelay(TXFLASHLOG);
    MSG_Enviar("\r\n [2] Estado tras intentar Write Enable (Fijate en WEL y BSY):\r\n", TestUart_1);
    Flash_Read_Component(flash);
    osDelay(TXFLASHLOG);

    osDelay(TXFLASHLOG);
    MSG_Enviar("\r\n [SPI FLASH] [Flash_Erase_All] Borrado total iniciado (Chip Erase). Espere...\r\n", TestUart_1);
    osDelay(TXFLASHLOG);

    uint8_t cmd = FLASH_CMD_CHIP_ERASE;
    HAL_StatusTypeDef estado_spi;

    if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
    	osDelay(TXFLASHLOG);
        MSG_Enviar("\r\n [SPI FLASH ERROR] [Flash_Erase_All] Timeout de Mutex.\r\n", TestUart_1);
        osDelay(TXFLASHLOG);
        return false;
    }


        uint8_t check_cmd = FLASH_CMD_READ_STATUS;
        uint8_t check_status = 0;

        HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
        HAL_SPI_Transmit(flash->hspi, &check_cmd, 1, WRITE_READ_NORMAL_VELOCITY);
        HAL_SPI_Receive(flash->hspi, &check_status, 1, WRITE_READ_NORMAL_VELOCITY);
        HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

        {
            char buf_dbg[64];
            snprintf(buf_dbg, sizeof(buf_dbg), "\r\n [DEBUG ANTES DE BORRAR] Status Reg: 0x%02X (WEL: %d)\r\n",
                     check_status, (check_status & 0x02) >> 1);
            MSG_Enviar(buf_dbg, TestUart_1);
        }
        // -------------------------------------------------------------

        // Ahora mandamos el comando de borrado
        HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
        estado_spi = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
        HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

        osMutexRelease(flash->spi_mutex);

        osDelay(100);

    if (estado_spi != HAL_OK) {
    	osDelay(TXFLASHLOG);
        MSG_Enviar("\r\n [SPI FLASH ERROR] [Flash_Erase_All] Fallo de hardware SPI al transmitir.\r\n", TestUart_1);
        osDelay(TXFLASHLOG);
        return false;
    }

    bool exito = Flash_Wait_For_Ready(flash, ERASE_ALL_FLASH);
    osDelay(TXFLASHLOG);

    flash->ocuped = false;

    if (exito) {
        // Reiniciar las particiones de forma segura
        for (int i = 0; i < PARTITION_COUNT; i++) {
            if (flash->partitions[i].is_configured) {
                flash->partitions[i].current_sector = flash->partitions[i].start_sector;
                flash->partitions[i].sector_offset  = 0;
            }
        }

        osDelay(TXFLASHLOG);
        MSG_Enviar(" [SPI FLASH] [Flash_Erase_All] Borrado completado con exito.\r\n", TestUart_1);
        osDelay(TXFLASHLOG);

        // --- CHIVATO DE MEMORIA INTELIGENTE ---
        {
			char *nombre_tarea = (char*)pcTaskGetName(NULL);

			UBaseType_t libres_words = uxTaskGetStackHighWaterMark(NULL);
			uint32_t libres_bytes = (uint32_t)(libres_words * 4);

			char buf_memoria[128];
			snprintf(buf_memoria, sizeof(buf_memoria),
					 "\r\n [DIAGNOSTICO] [Flash_Erase_All]\r\n"
					 "  |- Tarea Activa : %s\r\n"
					 "  \\- Margen Libre : %lu bytes\r\n",
					 nombre_tarea,
					 (unsigned long)libres_bytes);

			osDelay(TXFLASHLOG);
			MSG_Enviar(buf_memoria, TestUart_1);
			osDelay(TXFLASHLOG);
        }

        return true;
    } else {
        osDelay(TXFLASHLOG);
        MSG_Enviar(" [SPI FLASH ERROR] [Flash_Erase_All] Timeout crítico de borrado.\r\n", TestUart_1);
        osDelay(TXFLASHLOG);
        return false;
    }
}


bool Flash_Erase_All_For_Sector(SPI_Flash_TypeDef *flash, uint32_t sectors)
{
      if (flash == NULL) {
          return false;
      }

      // =========================================================================
      // DIAGNÓSTICO INICIAL (Lectura del estado en frío del componente)
      // =========================================================================
      osDelay(TXFLASHLOG);
      MSG_Enviar("\r\n --- DIAGNOSTICO PRE-BORRADO ---\r\n [1] Estado de la Flash en frio:\r\n", TestUart_1);
      osDelay(TXFLASHLOG);

      Flash_Read_Component(flash);

      osDelay(TXFLASHLOG);
      MSG_Enviar("\r\n [SPI FLASH] [Flash_Erase_All] Preparando secuencia de borrado...\r\n", TestUart_1);
      osDelay(TXFLASHLOG);

      // =========================================================================
      // DESBLOQUEO GLOBAL DE HARDWARE (Elimina la protección de fábrica en zona alta)
      // =========================================================================
      if (!Flash_Write_Enable(flash)) {
    	  osDelay(TXFLASHLOG);
          MSG_Enviar("\r\n [SPI FLASH ERROR] Fallo en Write Enable para Desbloqueo.\r\n", TestUart_1);
          osDelay(TXFLASHLOG);
          return false;
      }

      uint8_t cmd_unprotect = 0x01; // Comando Write Status Register
      uint8_t status_val[2] = {0x00, 0x00}; // Ponemos todos los bits BP a 0 (Sin protecciones de bloques)

      if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
    	  osDelay(TXFLASHLOG);
          MSG_Enviar("\r\n [SPI FLASH ERROR] Timeout de Mutex en desbloqueo.\r\n", TestUart_1);
          osDelay(TXFLASHLOG);
          return false;
      }

      HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
      HAL_SPI_Transmit(flash->hspi, &cmd_unprotect, 1, WRITE_READ_NORMAL_VELOCITY);
      HAL_SPI_Transmit(flash->hspi, status_val, 2, WRITE_READ_NORMAL_VELOCITY);
      HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

      osMutexRelease(flash->spi_mutex);

      // Esperar a que el chip procese internamente el desbloqueo global
      if (!Flash_Wait_For_Ready(flash, ERASE_ONE_SECTOR)) {
    	  osDelay(TXFLASHLOG);
          MSG_Enviar("\r\n [SPI FLASH ERROR] Timeout esperando desbloqueo global.\r\n", TestUart_1);
          osDelay(TXFLASHLOG);
          return false;
      }
      // =========================================================================

      // =========================================================================
      // BUCLE DE BARRIDO Y BORRADO SECTOR A SECTOR (4KB por bloque)
      // =========================================================================
      char buf_progreso[64];

      for (int sector = 0; sector < sectors; ++sector) {

          // Habilitación obligatoria de escritura previa a cada sector individual
          if (!Flash_Write_Enable(flash)) {
              osDelay(TXFLASHLOG);
              MSG_Enviar("\r\n [SPI FLASH ERROR] [Flash_Erase_All] Error en Write Enable.\r\n", TestUart_1);
              osDelay(TXFLASHLOG);
              return false;
          }

          uint8_t cmd = FLASH_CMD_SECTOR_ERASE;

          // Cálculo seguro de la dirección física de 24 bits (4096 bytes por sector)
          uint32_t dir_sector = (uint32_t)sector * 4096;

          uint8_t cmd_buff[4];
          cmd_buff[0] = cmd;
          cmd_buff[1] = (uint8_t)((dir_sector >> 16) & 0xFF);
          cmd_buff[2] = (uint8_t)((dir_sector >> 8) & 0xFF);
          cmd_buff[3] = (uint8_t)(dir_sector & 0xFF);

          // Protección del bus SPI mediante Mutex de FreeRTOS
          if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
              osDelay(TXFLASHLOG);
              MSG_Enviar("\r\n [SPI FLASH ERROR] Timeout de Mutex en borrado de sectores.\r\n", TestUart_1);
              osDelay(TXFLASHLOG);
              return false;
          }

          HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
          HAL_StatusTypeDef estado_spi = HAL_SPI_Transmit(flash->hspi, cmd_buff, 4, ERASE_ONE_SECTOR);
          HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

          osMutexRelease(flash->spi_mutex);

          osDelay(200);

          if (estado_spi != HAL_OK) {
              osDelay(TXFLASHLOG);
              MSG_Enviar("\r\n [SPI FLASH ERROR] Fallo de hardware SPI transmitiendo el sector.\r\n", TestUart_1);
              osDelay(TXFLASHLOG);
              return false;
          }

          // Espera activa controlada por el bit BSY del hardware de la memoria
          if (!Flash_Wait_For_Ready(flash, ERASE_ONE_SECTOR)) {
              osDelay(TXFLASHLOG);
              MSG_Enviar("\r\n [SPI FLASH ERROR] Timeout esperando a que el sector se borre.\r\n", TestUart_1);
              osDelay(TXFLASHLOG);
              return false;
          }

          osDelay(200);


		  snprintf(buf_progreso, sizeof(buf_progreso),
		 		 " [SPI FLASH] Borrando Sector %lu / %lu\r\n",
				 (unsigned long)sector,
				 (unsigned long)sectors);

		  osDelay(TXFLASHLOG);
		  MSG_Enviar(buf_progreso, TestUart_1);
		  osDelay(TXFLASHLOG);
}
      // =========================================================================

      // =========================================================================
      // REINICIO DE PARTICIONES LÓGICAS TRAS EL VACIADO FÍSICO TOTAL
      // =========================================================================
      for (int i = 0; i < PARTITION_COUNT; i++) {
          if (flash->partitions[i].is_configured) {
              flash->partitions[i].current_sector = flash->partitions[i].start_sector;
              flash->partitions[i].sector_offset  = 0;
          }
      }

      flash->ocuped = false;

      osDelay(TXFLASHLOG);
      MSG_Enviar(" [SPI FLASH] [Flash_Erase_All] Borrado por sectores completado con exito.\r\n", TestUart_1);
      osDelay(TXFLASHLOG);
      // =========================================================================

      // =========================================================================
      // DIAGNÓSTICO DE MEMORIA LIBRE (High Water Mark de la tarea actual)
      // =========================================================================
      {
          char *nombre_tarea = (char*)pcTaskGetName(NULL);
          UBaseType_t libres_words = uxTaskGetStackHighWaterMark(NULL);
          uint32_t libres_bytes = (uint32_t)(libres_words * 4);

          char buf_memoria[128];
          snprintf(buf_memoria, sizeof(buf_memoria),
                   "\r\n [DIAGNOSTICO] [Flash_Erase_All]\r\n"
                   "  |- Tarea Activa : %s\r\n"
                   "  \\- Margen Libre : %lu bytes\r\n",
                   nombre_tarea,
                   (unsigned long)libres_bytes);

          osDelay(TXFLASHLOG);
          MSG_Enviar(buf_memoria, TestUart_1);
          osDelay(TXFLASHLOG);
      }
      // =========================================================================

      return true;
}

bool Flash_Erase_One_Sector(SPI_Flash_TypeDef *flash, Flash_Partition_ID partition, uint32_t dirs)
{
      if (flash == NULL) {
          return false;
      }

      if(!Flash_Is_Sector_partition(flash,partition,dirs)){
          osDelay(TXFLASHLOG);
          MSG_Enviar("\r\n [SPI FLASH] [Flash_Erase_One_Sector] Fallo borrado por no estar particionado\r\n", TestUart_1);
          osDelay(TXFLASHLOG);

    	  return false;
      }

      if(!Flash_HaveData_Sector(flash,partition,dirs)){
		osDelay(TXFLASHLOG);
		MSG_Enviar("\r\n [SPI FLASH] [Flash_Erase_One_Sector] Sector vacio, no se realiza borrado \r\n", TestUart_1);
		osDelay(TXFLASHLOG);

	   return false;
	 }



      osDelay(TXFLASHLOG);
      MSG_Enviar("\r\n --- DIAGNOSTICO PRE-BORRADO ---\r\n [1] Estado de la Flash en frio:\r\n", TestUart_1);
      osDelay(TXFLASHLOG);

      Flash_Read_Component(flash);

      osDelay(TXFLASHLOG);
      MSG_Enviar("\r\n [SPI FLASH] [Flash_Erase_All] Preparando secuencia de borrado...\r\n", TestUart_1);
      osDelay(TXFLASHLOG);


      if (!Flash_Write_Enable(flash)) {
    	  osDelay(TXFLASHLOG);
          MSG_Enviar("\r\n [SPI FLASH ERROR] Fallo en Write Enable para Desbloqueo.\r\n", TestUart_1);
          osDelay(TXFLASHLOG);
          return false;
      }

      uint8_t cmd_unprotect = 0x01;
      uint8_t status_val[2] = {0x00, 0x00};

      if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
    	  osDelay(TXFLASHLOG);
          MSG_Enviar("\r\n [SPI FLASH ERROR] Timeout de Mutex en desbloqueo.\r\n", TestUart_1);
          osDelay(TXFLASHLOG);
          return false;
      }

      HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
      HAL_SPI_Transmit(flash->hspi, &cmd_unprotect, 1, WRITE_READ_NORMAL_VELOCITY);
      HAL_SPI_Transmit(flash->hspi, status_val, 2, WRITE_READ_NORMAL_VELOCITY);
      HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

      osMutexRelease(flash->spi_mutex);

      osDelay(10);


      if (!Flash_Wait_For_Ready(flash, ERASE_ONE_SECTOR)) {
    	  osDelay(TXFLASHLOG);
          MSG_Enviar("\r\n [SPI FLASH ERROR] Timeout esperando desbloqueo global.\r\n", TestUart_1);
          osDelay(TXFLASHLOG);
          return false;
      }

      char buf_progreso[64];


	  // Habilitación obligatoria de escritura previa a cada sector individual
	  if (!Flash_Write_Enable(flash)) {
		  osDelay(TXFLASHLOG);
		  MSG_Enviar("\r\n [SPI FLASH ERROR] [Flash_Erase_All] Error en Write Enable.\r\n", TestUart_1);
		  osDelay(TXFLASHLOG);
		  return false;
	  }

	  uint8_t cmd = FLASH_CMD_SECTOR_ERASE;
	  uint32_t absolute_address = dirs * FLASH_SECTION_SIZE;

	  uint8_t cmd_buff[4];
	  cmd_buff[0] = cmd;
	  cmd_buff[1] = (uint8_t)((absolute_address >> 16) & 0xFF);
	  cmd_buff[2] = (uint8_t)((absolute_address >> 8) & 0xFF);
	  cmd_buff[3] = (uint8_t)(absolute_address & 0xFF);


	  if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
		  osDelay(TXFLASHLOG);
		  MSG_Enviar("\r\n [SPI FLASH ERROR] Timeout de Mutex en borrado de sectores.\r\n", TestUart_1);
		  osDelay(TXFLASHLOG);
		  return false;
	  }

	  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	  HAL_StatusTypeDef estado_spi = HAL_SPI_Transmit(flash->hspi, cmd_buff, 4, ERASE_ONE_SECTOR);
	  HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

	  osMutexRelease(flash->spi_mutex);

	  osDelay(10);

	  if (estado_spi != HAL_OK) {
		  osDelay(TXFLASHLOG);
		  MSG_Enviar("\r\n [SPI FLASH ERROR] Fallo de hardware SPI transmitiendo el sector.\r\n", TestUart_1);
		  osDelay(TXFLASHLOG);
		  return false;
	  }


	  if (!Flash_Wait_For_Ready(flash, ERASE_ONE_SECTOR)) {
		  osDelay(TXFLASHLOG);
		  MSG_Enviar("\r\n [SPI FLASH ERROR] Timeout esperando a que el sector se borre.\r\n", TestUart_1);
		  osDelay(TXFLASHLOG);
		  return false;
	  }



	  snprintf(buf_progreso, sizeof(buf_progreso),
			 " [SPI FLASH] Borrando Sector %lu u\r\n",
			 (unsigned long)dirs);

	  osDelay(TXFLASHLOG);
	  MSG_Enviar(buf_progreso, TestUart_1);
	  osDelay(TXFLASHLOG);




      osDelay(TXFLASHLOG);
      MSG_Enviar(" [SPI FLASH] [Flash_Erase_All] Borrado por sector completado con exito.\r\n", TestUart_1);
      osDelay(TXFLASHLOG);
      // =========================================================================

      // =========================================================================
      // DIAGNÓSTICO DE MEMORIA LIBRE (High Water Mark de la tarea actual)
      // =========================================================================
      {
          char *nombre_tarea = (char*)pcTaskGetName(NULL);
          UBaseType_t libres_words = uxTaskGetStackHighWaterMark(NULL);
          uint32_t libres_bytes = (uint32_t)(libres_words * 4);

          char buf_memoria[128];
          snprintf(buf_memoria, sizeof(buf_memoria),
                   "\r\n [DIAGNOSTICO] [Flash_Erase_All]\r\n"
                   "  |- Tarea Activa : %s\r\n"
                   "  \\- Margen Libre : %lu bytes\r\n",
                   nombre_tarea,
                   (unsigned long)libres_bytes);

          osDelay(TXFLASHLOG);
          MSG_Enviar(buf_memoria, TestUart_1);
          osDelay(TXFLASHLOG);
      }
      // =========================================================================

      return true;
}
