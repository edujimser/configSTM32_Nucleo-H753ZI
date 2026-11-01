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

    if (!flash->init){
		return false;
	}

    HAL_StatusTypeDef estado_spi;
    HAL_StatusTypeDef estado_spi_2;
    uint8_t cmd = 0;

    osDelay(TXFLASHLOG);
    	   MSG_Enviar(
    		   "\r\n==========================================================\r\n"
    		   " [SPI FLASH] [Flash_Erase_All] BORADO COMPLETO\r\n"
    		   "==========================================================\r\n",
    		   TestUart_1
	);
	osDelay(TXFLASHLOG);

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
	// BORRADO
	// =========================================================================

	if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
		MSG_Enviar("\r\n [SPI FLASH FATAL] No se pudo secuestrar el bus SPI.\r\n", TestUart_1);
		return false;
	}




	cmd = FLASH_CMD_WRITE_ENABLE;

	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	estado_spi = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);


	if (estado_spi != HAL_OK) {
		MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando Write para Erase.\r\n", TestUart_1);
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
		MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando eliminar bloqueos zona alta.\r\n", TestUart_1);
		osMutexRelease(flash->spi_mutex);
		return false;
	}




	cmd = FLASH_CMD_WRITE_ENABLE;

	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	estado_spi = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);


	if (estado_spi != HAL_OK) {
		MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando Write para Erase.\r\n", TestUart_1);
		osMutexRelease(flash->spi_mutex);
		return false;
	}




	cmd = FLASH_CMD_CHIP_ERASE;

	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	estado_spi = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

	if (estado_spi != HAL_OK) {
		MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando Erase.\r\n", TestUart_1);
		osMutexRelease(flash->spi_mutex);
		return false;
	}

	// =========================================================================
	// COMPROBACION DE TIEMPO
	// =========================================================================


	osDelay(10);
	uint32_t timeUse = 0;
	char buf_msg[256];


	if (!Flash_Wait_Busy(flash, ERASE_ALL_WAIT_BUSY, &timeUse)) {
		osDelay(TXFLASHLOG);
		MSG_Enviar("\r\n [SPI FLASH FATAL] TIMEOUT: El chip se quedo bloqueado borrando.\r\n", TestUart_1);
		osDelay(TXFLASHLOG);

		osDelay(TXFLASHLOG);
		   MSG_Enviar(
			   "\r\n==========================================================\r\n"
			   " [SPI FLASH] [Flash_Erase_All] FIN BORADO COMPLETO\r\n"
			   "==========================================================\r\n",
			   TestUart_1
		   );
	   osDelay(TXFLASHLOG);

	   return false;
	} else {
		snprintf(buf_msg, sizeof(buf_msg),
				 "\r\n [SPI FLASH OK] Borrado Finalizado. Tiempo: %lu ms. Memoria virgen.\r\n",
				 (unsigned long)timeUse);

		MSG_Enviar(buf_msg, TestUart_1);
	}

	osDelay(TXFLASHLOG);
		   MSG_Enviar(
			   "\r\n==========================================================\r\n"
			   " [SPI FLASH] [Flash_Erase_All] FIN BORADO COMPLETO\r\n"
			   "==========================================================\r\n",
			   TestUart_1
	);


	osDelay(TXFLASHLOG * 15);

	// =========================================================================
	// RESET
	// =========================================================================


	__disable_irq();
	NVIC_SystemReset();

   return true;

}

bool Flash_Erase_One_Hex_For_Sector(SPI_Flash_TypeDef *flash, uint32_t addr)
{
    if (flash == NULL) {
        return false;
    }

    if (!flash->init || !flash->load){
		return false;
	}

    HAL_StatusTypeDef estado_spi;
    HAL_StatusTypeDef estado_spi_2;

    osDelay(TXFLASHLOG);
    	   MSG_Enviar(
    		   "\r\n==========================================================\r\n"
    		   " [SPI FLASH] [Flash_Erase_One_Hex_For_Sector] BORADO POR SECTOR\r\n"
    		   "==========================================================\r\n",

    		   TestUart_1
	);
	osDelay(TXFLASHLOG);


	// =========================================================================
	// COMPROBACIONES
	// =========================================================================
	bool protocol = true;

	if((addr >= GET_FLASH_CAPCITY) || ((addr % FLASH_SECTION_SIZE) != 0)){
		protocol = false;
	}

	if (!protocol) {
		osDelay(TXFLASHLOG);
		MSG_Enviar("\r\n [SPI FLASH] [Flash_Erase_One_Hex_For_Sector] Fuera de limites o dirección incorrecta (No inicial)\r\n", TestUart_1);
		osDelay(TXFLASHLOG);

		osDelay(TXFLASHLOG);
		MSG_Enviar(
		   "\r\n==========================================================\r\n"
		   " [SPI FLASH] [Flash_Erase_One_Hex_For_Sector] BORADO POR SECTOR\r\n"
		   "==========================================================\r\n",
		   TestUart_1);
	   osDelay(TXFLASHLOG);

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
	MSG_Enviar("\r\n [SPI FLASH] [Flash_Erase_One_For_Sector] Preparando secuencia de borrado...\r\n", TestUart_1);
	osDelay(TXFLASHLOG);

	// =========================================================================
	// BORRADO
	// =========================================================================

	if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
		MSG_Enviar("\r\n [SPI FLASH FATAL] No se pudo secuestrar el bus SPI\r\n", TestUart_1);
		return false;
	}


	uint8_t cmd = FLASH_CMD_WRITE_ENABLE;

	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	estado_spi = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);


	if (estado_spi != HAL_OK) {
		MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando Write para Erase\r\n", TestUart_1);
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
		MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando eliminar bloqueos zona alta\r\n", TestUart_1);
		osMutexRelease(flash->spi_mutex);
		return false;
	}



	cmd = FLASH_CMD_WRITE_ENABLE;

	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	estado_spi = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);


	if (estado_spi != HAL_OK) {
		MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando Write para Erase\r\n", TestUart_1);
		osMutexRelease(flash->spi_mutex);
		return false;
	}



	uint8_t cmd_erase[4];
	cmd_erase[0] = FLASH_CMD_SECTOR_ERASE;
	cmd_erase[1] = (uint8_t)((addr >> 16) & 0XFF);
	cmd_erase[2] = (uint8_t)((addr >> 8)  & 0xFF);
	cmd_erase[3] = (uint8_t)(addr & 0xFF);

	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	estado_spi = HAL_SPI_Transmit(flash->hspi, cmd_erase, 4, WRITE_READ_NORMAL_VELOCITY);
	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);


	if (estado_spi != HAL_OK) {
		MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando Write para Erase\r\n", TestUart_1);
		osMutexRelease(flash->spi_mutex);
		return false;
	}


	// =========================================================================
	// COMPROBACION DE TIEMPO
	// =========================================================================
	uint32_t timeUse = 0;
	char buf_msg[256];


	if (!Flash_Wait_Busy(flash, ERASE_ONE_SECTOR, &timeUse)) {
		MSG_Enviar("\r\n [SPI FLASH FATAL] TIMEOUT: El chip se quedo bloqueado borrando\r\n", TestUart_1);
	} else {
		snprintf(buf_msg, sizeof(buf_msg),
				 "\r\n [SPI FLASH OK] Borrado Finalizado. Tiempo: %lu ms\r\n",
				 (unsigned long)timeUse);

		osDelay(TXFLASHLOG);
		MSG_Enviar(buf_msg, TestUart_1);
		osDelay(TXFLASHLOG);
	}


	// =========================================================================
	// LIBERACION
	// =========================================================================

   osMutexRelease(flash->spi_mutex);

   Flash_Complement_Data_Thread(FLASH_DEBUG_THREAD_SIZE);

   osDelay(TXFLASHLOG);
   MSG_Enviar(
	   "\r\n==========================================================\r\n"
	   " [SPI FLASH] [Flash_Erase_One_Hex_For_Sector] BORADO POR SECTOR\r\n"
	   "==========================================================\r\n",
	   TestUart_1);
   osDelay(TXFLASHLOG);

   return true;
}

bool Flash_Erase_One_Sector_For_Sector(SPI_Flash_TypeDef *flash, uint32_t sector)
{
    if (flash == NULL) {
        return false;
    }

    if (!flash->init || !flash->load){
		return false;
	}

    HAL_StatusTypeDef estado_spi;
    HAL_StatusTypeDef estado_spi_2;
    uint32_t addr = sector * FLASH_SECTION_SIZE;

    osDelay(TXFLASHLOG);
    	   MSG_Enviar(
    		   "\r\n==========================================================\r\n"
    		   " [SPI FLASH] [Flash_Erase_One_sector_For_Sector] BORADO POR SECTOR\r\n"
    		   "==========================================================\r\n",

    		   TestUart_1
	);
	osDelay(TXFLASHLOG);

	// =========================================================================
	// COMPROBACIONES
	// =========================================================================
	bool protocol = true;

	if((addr >= GET_FLASH_CAPCITY) || ((addr % FLASH_SECTION_SIZE) != 0)){
		protocol = false;
	}

	if (!protocol) {
		osDelay(TXFLASHLOG);
		MSG_Enviar("\r\n [SPI FLASH] [Flash_Erase_One_sector_For_Sector] Fuera de limites o dirección incorrecta (No inicial)\r\n", TestUart_1);
		osDelay(TXFLASHLOG);

		osDelay(TXFLASHLOG);
		MSG_Enviar(
		   "\r\n==========================================================\r\n"
		   " [SPI FLASH] [Flash_Erase_One_sector_For_Sector] BORADO POR SECTOR\r\n"
		   "==========================================================\r\n",
		   TestUart_1);
	   osDelay(TXFLASHLOG);

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
	MSG_Enviar("\r\n [SPI FLASH] [Flash_Erase_One_For_Sector] Preparando secuencia de borrado...\r\n", TestUart_1);
	osDelay(TXFLASHLOG);

	// =========================================================================
	// BORRADO
	// =========================================================================

	if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
		MSG_Enviar("\r\n [SPI FLASH FATAL] No se pudo secuestrar el bus SPI\r\n", TestUart_1);
		return false;
	}


	uint8_t cmd = FLASH_CMD_WRITE_ENABLE;

	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	estado_spi = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);


	if (estado_spi != HAL_OK) {
		MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando Write para Erase\r\n", TestUart_1);
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
		MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando eliminar bloqueos zona alta\r\n", TestUart_1);
		osMutexRelease(flash->spi_mutex);
		return false;
	}



	cmd = FLASH_CMD_WRITE_ENABLE;

	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	estado_spi = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);


	if (estado_spi != HAL_OK) {
		MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando Write para Erase\r\n", TestUart_1);
		osMutexRelease(flash->spi_mutex);
		return false;
	}



	uint8_t cmd_erase[4];
	cmd_erase[0] = FLASH_CMD_SECTOR_ERASE;
	cmd_erase[1] = (uint8_t)((addr >> 16) & 0XFF);
	cmd_erase[2] = (uint8_t)((addr >> 8)  & 0xFF);
	cmd_erase[3] = (uint8_t)(addr & 0xFF);

	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	estado_spi = HAL_SPI_Transmit(flash->hspi, cmd_erase, 4, WRITE_READ_NORMAL_VELOCITY);
	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);


	if (estado_spi != HAL_OK) {
		MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando Write para Erase\r\n", TestUart_1);
		osMutexRelease(flash->spi_mutex);
		return false;
	}


	// =========================================================================
	// COMPROBACION DE TIEMPO
	// =========================================================================
	uint32_t timeUse = 0;
	char buf_msg[256];


	if (!Flash_Wait_Busy(flash, ERASE_ONE_SECTOR, &timeUse)) {
		MSG_Enviar("\r\n [SPI FLASH FATAL] TIMEOUT: El chip se quedo bloqueado borrando\r\n", TestUart_1);
	} else {
		snprintf(buf_msg, sizeof(buf_msg),
				 "\r\n [SPI FLASH OK] Borrado Finalizado. Tiempo: %lu ms\r\n",
				 (unsigned long)timeUse);

		osDelay(TXFLASHLOG);
		MSG_Enviar(buf_msg, TestUart_1);
		osDelay(TXFLASHLOG);
	}


	// =========================================================================
	// LIBERACION
	// =========================================================================

   osMutexRelease(flash->spi_mutex);

   Flash_Complement_Data_Thread(FLASH_DEBUG_THREAD_SIZE);

   osDelay(TXFLASHLOG);
   MSG_Enviar(
	   "\r\n==========================================================\r\n"
	   " [SPI FLASH] [Flash_Erase_One_sector_For_Sector] BORADO POR SECTOR\r\n"
	   "==========================================================\r\n",
	   TestUart_1);
   osDelay(TXFLASHLOG);

   return true;
}

bool Flash_Erase_Range_Hex_For_Sector(SPI_Flash_TypeDef *flash, uint32_t addrInit, uint32_t addrFinal)
{
    if (flash == NULL) {
        return false;
    }

    if (!flash->init || !flash->load){
		return false;
	}

    HAL_StatusTypeDef estado_spi;
    HAL_StatusTypeDef estado_spi_2;

    osDelay(TXFLASHLOG);
    	   MSG_Enviar(
    		   "\r\n==========================================================\r\n"
    		   " [SPI FLASH] [Flash_Erase_Range_Hex_For_Sector] BORADO POR SECTOR\r\n"
    		   "==========================================================\r\n",

    		   TestUart_1
	);
	osDelay(TXFLASHLOG);

	// =========================================================================
	// COMPROBACIONES
	// =========================================================================
	bool protocol = true;

	if((addrInit >= GET_FLASH_CAPCITY)        ||
	  ((addrInit % FLASH_SECTION_SIZE) != 0)  ||
	  (addrFinal >= GET_FLASH_CAPCITY)        ||
	  ((addrFinal % FLASH_SECTION_SIZE) != 0) ||
	  (addrInit > addrFinal)
	){
		protocol = false;
	}

	if (!protocol) {
		osDelay(TXFLASHLOG);
		MSG_Enviar("\r\n [SPI FLASH] [Flash_Erase_Range_Hex_For_Sector] Fuera de limites o dirección incorrecta (No inicial o  final)\r\n", TestUart_1);
		osDelay(TXFLASHLOG);

		osDelay(TXFLASHLOG);
		MSG_Enviar(
		   "\r\n==========================================================\r\n"
		   " [SPI FLASH] [Flash_Erase_Range_Hex_For_Sector] BORRADO POR RANGO HEX\r\n"
		   "==========================================================\r\n",
		   TestUart_1);
	   osDelay(TXFLASHLOG);

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
	MSG_Enviar("\r\n [SPI FLASH] [Flash_Erase_Range_Hex_For_Sector] Preparando secuencia de borrado...\r\n", TestUart_1);
	osDelay(TXFLASHLOG);

	// =========================================================================
	// BORRADO
	// =========================================================================

	if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
		MSG_Enviar("\r\n [SPI FLASH FATAL] No se pudo secuestrar el bus SPI\r\n", TestUart_1);
		return false;
	}


	 uint8_t cmd = FLASH_CMD_WRITE_ENABLE;

	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	estado_spi = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);


	if (estado_spi != HAL_OK) {
		MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando Write para Erase\r\n", TestUart_1);
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
		MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando eliminar bloqueos zona alta\r\n", TestUart_1);
		osMutexRelease(flash->spi_mutex);
		return false;
	}


	uint32_t dummy_time;
	Flash_Wait_Busy(flash, ERASE_ONE_SECTOR, &dummy_time);

	uint32_t totalTimeUse = 0;
	uint32_t timeUse_sector = 0;

	for (uint32_t i = addrInit;  i <= addrFinal;  i = i + FLASH_SECTION_SIZE) {

		cmd = FLASH_CMD_WRITE_ENABLE;

		HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
		estado_spi = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
		HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);


		if (estado_spi != HAL_OK) {
			MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando Write para Erase\r\n", TestUart_1);
			osMutexRelease(flash->spi_mutex);
			return false;
		}



		uint8_t cmd_erase[4];
		cmd_erase[0] = FLASH_CMD_SECTOR_ERASE;
		cmd_erase[1] = (uint8_t)((i >> 16) & 0XFF);
		cmd_erase[2] = (uint8_t)((i >> 8)  & 0xFF);
		cmd_erase[3] = (uint8_t)(i & 0xFF);

		HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
		estado_spi = HAL_SPI_Transmit(flash->hspi, cmd_erase, 4, WRITE_READ_NORMAL_VELOCITY);
		HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);


		if (estado_spi != HAL_OK) {
			MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando Write para Erase\r\n", TestUart_1);
			osMutexRelease(flash->spi_mutex);
			return false;
		}

		if (!Flash_Wait_Busy(flash, ERASE_ONE_SECTOR, &timeUse_sector)) {
			MSG_Enviar("\r\n [SPI FLASH FATAL] TIMEOUT: El chip se quedo bloqueado borrando\r\n", TestUart_1);
			osMutexRelease(flash->spi_mutex);
			return false;
		}

		 totalTimeUse += timeUse_sector;

	}

	// =========================================================================
	// COMPROBACION DE TIEMPO
	// =========================================================================
	char buf_msg[256];
	    snprintf(buf_msg, sizeof(buf_msg),
	             "\r\n [SPI FLASH OK] Rango Borrado Finalizado. Tiempo Total: %lu ms\r\n",
	             (unsigned long)totalTimeUse);

	    osDelay(TXFLASHLOG);
	    MSG_Enviar(buf_msg, TestUart_1);
	    osDelay(TXFLASHLOG);


	// =========================================================================
	// LIBERACION
	// =========================================================================

   osMutexRelease(flash->spi_mutex);

   Flash_Complement_Data_Thread(FLASH_DEBUG_THREAD_SIZE);

   osDelay(TXFLASHLOG);
   MSG_Enviar(
	   "\r\n==========================================================\r\n"
	   " [SPI FLASH] [Flash_Erase_Range_Hex_For_Sector] FIN \r\n"
	   "==========================================================\r\n",
	   TestUart_1);
   osDelay(TXFLASHLOG);

   return true;
}

bool Flash_Erase_Range_Sector_For_Sector(SPI_Flash_TypeDef *flash, uint32_t sectorInit, uint32_t SectorFinal)
{
    if (flash == NULL) {
        return false;
    }

    if (!flash->init || !flash->load){
		return false;
	}

    HAL_StatusTypeDef estado_spi;
    HAL_StatusTypeDef estado_spi_2;

    osDelay(TXFLASHLOG);
    	   MSG_Enviar(
    		   "\r\n==========================================================\r\n"
    		   " [SPI FLASH] [Flash_Erase_Range_Sector_For_Sector] BORADO POR SECTOR\r\n"
    		   "==========================================================\r\n",

    		   TestUart_1
	);
	osDelay(TXFLASHLOG);

	// =========================================================================
	// COMPROBACIONES
	// =========================================================================
	bool protocol = true;
	uint32_t addrInit = sectorInit * FLASH_SECTION_SIZE;
	uint32_t addrFinal = SectorFinal * FLASH_SECTION_SIZE;

	if((addrInit >= GET_FLASH_CAPCITY)        ||
	  ((addrInit % FLASH_SECTION_SIZE) != 0)  ||
	  (addrFinal >= GET_FLASH_CAPCITY)        ||
	  ((addrFinal % FLASH_SECTION_SIZE) != 0) ||
	  (addrInit > addrFinal)
	){
		protocol = false;
	}

	if (!protocol) {
		osDelay(TXFLASHLOG);
		MSG_Enviar("\r\n [SPI FLASH] [Flash_Erase_Range_Sector_For_Sector] Fuera de limites o dirección incorrecta (No inicial o  final)\r\n", TestUart_1);
		osDelay(TXFLASHLOG);

		osDelay(TXFLASHLOG);
		MSG_Enviar(
		   "\r\n==========================================================\r\n"
		   " [SPI FLASH] [Flash_Erase_Range_Sector_For_Sector] BORRADO POR RANGO HEX\r\n"
		   "==========================================================\r\n",
		   TestUart_1);
	   osDelay(TXFLASHLOG);

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
	MSG_Enviar("\r\n [SPI FLASH] [Flash_Erase_Range_Sector_For_Sector] Preparando secuencia de borrado...\r\n", TestUart_1);
	osDelay(TXFLASHLOG);

	// =========================================================================
	// BORRADO
	// =========================================================================

	if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
		MSG_Enviar("\r\n [SPI FLASH FATAL] No se pudo secuestrar el bus SPI\r\n", TestUart_1);
		return false;
	}


	 uint8_t cmd = FLASH_CMD_WRITE_ENABLE;

	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	estado_spi = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);


	if (estado_spi != HAL_OK) {
		MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando Write para Erase\r\n", TestUart_1);
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
		MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando eliminar bloqueos zona alta\r\n", TestUart_1);
		osMutexRelease(flash->spi_mutex);
		return false;
	}


	uint32_t dummy_time;
	Flash_Wait_Busy(flash, ERASE_ONE_SECTOR, &dummy_time);

	uint32_t totalTimeUse = 0;
	uint32_t timeUse_sector = 0;

	for (uint32_t i = addrInit;  i <= addrFinal;  i = i + FLASH_SECTION_SIZE) {

		cmd = FLASH_CMD_WRITE_ENABLE;

		HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
		estado_spi = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
		HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);


		if (estado_spi != HAL_OK) {
			MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando Write para Erase\r\n", TestUart_1);
			osMutexRelease(flash->spi_mutex);
			return false;
		}



		uint8_t cmd_erase[4];
		cmd_erase[0] = FLASH_CMD_SECTOR_ERASE;
		cmd_erase[1] = (uint8_t)((i >> 16) & 0XFF);
		cmd_erase[2] = (uint8_t)((i >> 8)  & 0xFF);
		cmd_erase[3] = (uint8_t)(i & 0xFF);

		HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
		estado_spi = HAL_SPI_Transmit(flash->hspi, cmd_erase, 4, WRITE_READ_NORMAL_VELOCITY);
		HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);


		if (estado_spi != HAL_OK) {
			MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando Write para Erase\r\n", TestUart_1);
			osMutexRelease(flash->spi_mutex);
			return false;
		}

		if (!Flash_Wait_Busy(flash, ERASE_ONE_SECTOR, &timeUse_sector)) {
			MSG_Enviar("\r\n [SPI FLASH FATAL] TIMEOUT: El chip se quedo bloqueado borrando\r\n", TestUart_1);
			osMutexRelease(flash->spi_mutex);
			return false;
		}

		 totalTimeUse += timeUse_sector;

	}

	// =========================================================================
	// COMPROBACION DE TIEMPO
	// =========================================================================
	char buf_msg[256];
	    snprintf(buf_msg, sizeof(buf_msg),
	             "\r\n [SPI FLASH OK] Rango Borrado Finalizado. Tiempo Total: %lu ms\r\n",
	             (unsigned long)totalTimeUse);

	    osDelay(TXFLASHLOG);
	    MSG_Enviar(buf_msg, TestUart_1);
	    osDelay(TXFLASHLOG);


	// =========================================================================
	// LIBERACION
	// =========================================================================

   osMutexRelease(flash->spi_mutex);

   Flash_Complement_Data_Thread(FLASH_DEBUG_THREAD_SIZE);

   osDelay(TXFLASHLOG);
   MSG_Enviar(
	   "\r\n==========================================================\r\n"
	   " [SPI FLASH] [Flash_Erase_Range_Sector_For_Sector] FIN \r\n"
	   "==========================================================\r\n",
	   TestUart_1);
   osDelay(TXFLASHLOG);

   return true;
}

bool Flash_Erase_Partition(SPI_Flash_TypeDef *flash, Flash_Partition_ID partition)
{
    if (flash == NULL) {
        return false;
    }

    if (!flash->init || !flash->load){
		return false;
	}

    HAL_StatusTypeDef estado_spi;
    HAL_StatusTypeDef estado_spi_2;

    osDelay(TXFLASHLOG);
    	   MSG_Enviar(
    		   "\r\n==========================================================\r\n"
    		   " [SPI FLASH] [Flash_Erase_Partition] BORADO POR SECTOR\r\n"
    		   "==========================================================\r\n",

    		   TestUart_1
	);
	osDelay(TXFLASHLOG);

	// =========================================================================
	// COMPROBACIONES
	// =========================================================================
	bool protocol = true;
	uint32_t addrInit = flash->partitions[partition].start_sector;
	uint32_t addrFinal = flash->partitions[partition].end_sector;

	if((addrInit >= GET_FLASH_CAPCITY)        ||
	  ((addrInit % FLASH_SECTION_SIZE) != 0)  ||
	  (addrFinal >= GET_FLASH_CAPCITY)        ||
	  ((addrFinal % FLASH_SECTION_SIZE) != 0) ||
	  (addrInit > addrFinal)
	){
		protocol = false;
	}

	if (!protocol) {
		osDelay(TXFLASHLOG);
		MSG_Enviar("\r\n [SPI FLASH] [Flash_Erase_Partition] Fuera de limites o dirección incorrecta (No inicial o  final)\r\n", TestUart_1);
		osDelay(TXFLASHLOG);

		osDelay(TXFLASHLOG);
		MSG_Enviar(
		   "\r\n==========================================================\r\n"
		   " [SPI FLASH] [Flash_Erase_Partition] BORRADO POR RANGO HEX\r\n"
		   "==========================================================\r\n",
		   TestUart_1);
	   osDelay(TXFLASHLOG);

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
	MSG_Enviar("\r\n [SPI FLASH] [Flash_Erase_Partition] Preparando secuencia de borrado...\r\n", TestUart_1);
	osDelay(TXFLASHLOG);

	// =========================================================================
	// BORRADO
	// =========================================================================

	if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
		MSG_Enviar("\r\n [SPI FLASH FATAL] No se pudo secuestrar el bus SPI\r\n", TestUart_1);
		return false;
	}


	 uint8_t cmd = FLASH_CMD_WRITE_ENABLE;

	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	estado_spi = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);


	if (estado_spi != HAL_OK) {
		MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando Write para Erase\r\n", TestUart_1);
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
		MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando eliminar bloqueos zona alta\r\n", TestUart_1);
		osMutexRelease(flash->spi_mutex);
		return false;
	}


	uint32_t dummy_time;
	Flash_Wait_Busy(flash, ERASE_ONE_SECTOR, &dummy_time);

	uint32_t totalTimeUse = 0;
	uint32_t timeUse_sector = 0;

	for (uint32_t i = addrInit;  i <= addrFinal;  i = i + FLASH_SECTION_SIZE) {

		cmd = FLASH_CMD_WRITE_ENABLE;

		HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
		estado_spi = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);
		HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);


		if (estado_spi != HAL_OK) {
			MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando Write para Erase\r\n", TestUart_1);
			osMutexRelease(flash->spi_mutex);
			return false;
		}



		uint8_t cmd_erase[4];
		cmd_erase[0] = FLASH_CMD_SECTOR_ERASE;
		cmd_erase[1] = (uint8_t)((i >> 16) & 0XFF);
		cmd_erase[2] = (uint8_t)((i >> 8)  & 0xFF);
		cmd_erase[3] = (uint8_t)(i & 0xFF);

		HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
		estado_spi = HAL_SPI_Transmit(flash->hspi, cmd_erase, 4, WRITE_READ_NORMAL_VELOCITY);
		HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);


		if (estado_spi != HAL_OK) {
			MSG_Enviar("\r\n [SPI FLASH FATAL] Fallo al enviar comando Write para Erase\r\n", TestUart_1);
			osMutexRelease(flash->spi_mutex);
			return false;
		}

		if (!Flash_Wait_Busy(flash, ERASE_ONE_SECTOR, &timeUse_sector)) {
			MSG_Enviar("\r\n [SPI FLASH FATAL] TIMEOUT: El chip se quedo bloqueado borrando\r\n", TestUart_1);
			osMutexRelease(flash->spi_mutex);
			return false;
		}

		 totalTimeUse += timeUse_sector;

	}

	// =========================================================================
	// COMPROBACION DE TIEMPO
	// =========================================================================
	char buf_msg[256];
	    snprintf(buf_msg, sizeof(buf_msg),
	             "\r\n [SPI FLASH OK] Rango Borrado Finalizado. Tiempo Total: %lu ms\r\n",
	             (unsigned long)totalTimeUse);

	    osDelay(TXFLASHLOG);
	    MSG_Enviar(buf_msg, TestUart_1);
	    osDelay(TXFLASHLOG);


	// =========================================================================
	// LIBERACION
	// =========================================================================

   osMutexRelease(flash->spi_mutex);

   Flash_Complement_Data_Thread(FLASH_DEBUG_THREAD_SIZE);

   osDelay(TXFLASHLOG);
   MSG_Enviar(
	   "\r\n==========================================================\r\n"
	   " [SPI FLASH] [Flash_Erase_Partition] FIN BORRADO PARTITION\r\n"
	   "==========================================================\r\n",
	   TestUart_1);
   osDelay(TXFLASHLOG);

   return true;
}




