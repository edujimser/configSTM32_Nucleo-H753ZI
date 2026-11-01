#include <string.h>
#include <stdio.h>

#include "cmsis_os.h"

#include "SPI_Flash_Memory/SPI_Flash_Memory.h"
#include "MsgUART/TxUart/TxUart.h"


extern I_TestUart TestUart_1;


bool Flash_read_One_Sector_Search(SPI_Flash_TypeDef *flash, uint32_t addr, uint32_t sizeSearchData, uint8_t *data){
if (flash == NULL) {
       return false;
   }

   if (!flash->init || !flash->load){
	   return false;
   }

   if(addr > GET_FLASH_CAPCITY){
	   osDelay(TXFLASHLOG);
	   MSG_Enviar("\r\n [SPI FLASH ERROR][Flash_read_One_Sector_Search] Fallo en direccióin fuera de limites.\r\n", TestUart_1);
	   osDelay(TXFLASHLOG);
	   return false;
   }

   if(sizeSearchData > FLASH_PAGE_SIZE){
	   osDelay(TXFLASHLOG);
	   MSG_Enviar("\r\n [SPI FLASH ERROR][Flash_read_One_Sector_Search] No se puede superar el limite de una pagina para la busqueda.\r\n", TestUart_1);
	   osDelay(TXFLASHLOG);
	   return false;
   }


   uint8_t tx_cmd[4];
   tx_cmd[0] = FLASH_CMD_READ;
   tx_cmd[1] = (uint8_t)((addr >> 16) & 0xFF);
   tx_cmd[2] = (uint8_t)((addr >> 8) & 0xFF);
   tx_cmd[3] = (uint8_t)(addr & 0xFF);

   if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
	  osDelay(TXFLASHLOG);
	  MSG_Enviar("\r\n [SPI FLASH OCUPED][Flash_read_One_Sector_Search] SPI secuestrado.\r\n", TestUart_1);
	  osDelay(TXFLASHLOG);
	  return false;
	}

   HAL_StatusTypeDef status_tx = HAL_ERROR;
   HAL_StatusTypeDef status_rx = HAL_ERROR;

   HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
   status_tx = HAL_SPI_Transmit(flash->hspi, tx_cmd, 4, WRITE_READ_NORMAL_VELOCITY);
   if(status_tx == HAL_OK) status_rx = HAL_SPI_Receive(flash->hspi, data, sizeSearchData, WRITE_READ_NORMAL_VELOCITY);
   HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

   osMutexRelease(flash->spi_mutex);

   if (status_tx != HAL_OK) {
	   char buf_msg[200];
	   snprintf(buf_msg, sizeof(buf_msg),
				"\r\n [SPI FLASH ERROR] [%s] Fallo HAL_SPI_Transmit.\r\n"
				"  |- HAL Status : %d\r\n"
				"  \\- Bytes req. : %lu\r\n",
				__func__, status_tx, (unsigned long)(sizeSearchData + 4));

	   osDelay(TXFLASHLOG);
	   MSG_Enviar(buf_msg, TestUart_1);
	   osDelay(TXFLASHLOG);
	   return false;
   }

   if (status_rx != HAL_OK) {
   	   char buf_msg[200];
   	   snprintf(buf_msg, sizeof(buf_msg),
   				"\r\n [SPI FLASH ERROR] [%s] Fallo HAL_SPI_Receive.\r\n"
   				"  |- HAL Status : %d\r\n"
   				"  \\- Bytes req. : %lu\r\n",
   				__func__, status_rx, (unsigned long)(sizeSearchData + 4));

   	   osDelay(TXFLASHLOG);
   	   MSG_Enviar(buf_msg, TestUart_1);
   	   osDelay(TXFLASHLOG);
   	   return false;
      }

   Flash_Complement_Data_Thread(FLASH_DEBUG_THREAD_SIZE);

   return true;
}

bool Flash_read_One_Sector_Search_No_Mutex(SPI_Flash_TypeDef *flash, uint32_t addr, uint32_t sizeSearchData, uint8_t *data){
if (flash == NULL) {
       return false;
   }

   if (!flash->init || !flash->load){
	   return false;
   }

   if(addr > GET_FLASH_CAPCITY){
	   osDelay(TXFLASHLOG);
	   MSG_Enviar("\r\n [SPI FLASH ERROR][Flash_read_One_Sector_Search] Fallo en direccióin fuera de limites.\r\n", TestUart_1);
	   osDelay(TXFLASHLOG);
	   return false;
   }

   if(sizeSearchData > FLASH_PAGE_SIZE){
	   osDelay(TXFLASHLOG);
	   MSG_Enviar("\r\n [SPI FLASH ERROR][Flash_read_One_Sector_Search] No se puede superar el limite de una pagina para la busqueda.\r\n", TestUart_1);
	   osDelay(TXFLASHLOG);
	   return false;
   }


   uint8_t tx_cmd[4];
   tx_cmd[0] = FLASH_CMD_READ;
   tx_cmd[1] = (uint8_t)((addr >> 16) & 0xFF);
   tx_cmd[2] = (uint8_t)((addr >> 8) & 0xFF);
   tx_cmd[3] = (uint8_t)(addr & 0xFF);



   HAL_StatusTypeDef status_tx = HAL_ERROR;
   HAL_StatusTypeDef status_rx = HAL_ERROR;

   HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
   status_tx = HAL_SPI_Transmit(flash->hspi, tx_cmd, 4, WRITE_READ_NORMAL_VELOCITY);
   if(status_tx == HAL_OK) status_rx = HAL_SPI_Receive(flash->hspi, data, sizeSearchData, WRITE_READ_NORMAL_VELOCITY);
   HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);



   if (status_tx != HAL_OK) {
	   char buf_msg[200];
	   snprintf(buf_msg, sizeof(buf_msg),
				"\r\n [SPI FLASH ERROR] [%s] Fallo HAL_SPI_Transmit.\r\n"
				"  |- HAL Status : %d\r\n"
				"  \\- Bytes req. : %lu\r\n",
				__func__, status_tx, (unsigned long)(sizeSearchData + 4));

	   osDelay(TXFLASHLOG);
	   MSG_Enviar(buf_msg, TestUart_1);
	   osDelay(TXFLASHLOG);
	   return false;
   }

   if (status_rx != HAL_OK) {
   	   char buf_msg[200];
   	   snprintf(buf_msg, sizeof(buf_msg),
   				"\r\n [SPI FLASH ERROR] [%s] Fallo HAL_SPI_Receive.\r\n"
   				"  |- HAL Status : %d\r\n"
   				"  \\- Bytes req. : %lu\r\n",
   				__func__, status_tx, (unsigned long)(sizeSearchData + 4));

   	   osDelay(TXFLASHLOG);
   	   MSG_Enviar(buf_msg, TestUart_1);
   	   osDelay(TXFLASHLOG);
   	   return false;
      }

   Flash_Complement_Data_Thread(FLASH_DEBUG_THREAD_SIZE);

   return true;
}
