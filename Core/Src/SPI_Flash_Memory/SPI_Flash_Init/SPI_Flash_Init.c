#include <string.h>
#include <stdio.h>

#include "cmsis_os.h"

#include "SPI_Flash_Memory/SPI_Flash_Memory.h"
#include "MsgUART/TxUart/TxUart.h"


extern I_TestUart Uart1;

SPI_Flash_TypeDef spi_flash_memory_1;

/*
 * ==========================================================================================
 *                                DIAGRAMA DE FLUJO: Flash_Init()
 * ==========================================================================================
 *
 *                                 ┌────────────────────────┐
 *                                 │   INICIO: Flash_Init   │
 *                                 └───────────┬────────────┘
 *                                             ▼
 *                            [1]  ¿Punteros válidos? (flash, SPI, CS)
 *                                 ├── NO ──► [ ABORTAR (ERR PASO 1) ]
 *                                 ▼ SÍ
 *                            [2]  ¿Mutex SPI creado correctamente?
 *                                 ├── NO ──► [ ABORTAR (ERR MUTEX) ]
 *                                 ▼ SÍ
 *                                 ┌────────────────────────┐
 *                                 │ Enviar Comandos de     │
 *                                 │ Reset SW al Chip       │
 *                                 └───────────┬────────────┘
 *                                             ▼
 *                            [3]  ¿Reset SW ejecutado con Éxito?
 *                                 │
 *                                 ├── NO ──► [ Flash_Init_Reset_SPI(0x1) ]
 *                                 │          ├── Fallo ────► [ ABORTAR (ERR RST 1) ]
 *                                 │          └── Recuperado ─┐
 *                                 ▼ SÍ                       │
 *                                 ◄──────────────────────────┘
 *                                 ┌────────────────────────┐
 *                                 │ Leer ID del Chip       │
 *                                 │ (JEDEC ID)             │
 *                                 └───────────┬────────────┘
 *                                             ▼
 *                            [4]  ¿ID detectado y soportado?
 *                                 │
 *                                 ├── NO ──► [ Flash_Init_Reset_SPI(0x2) ]
 *                                 │          ├── Fallo ────► [ ABORTAR (ERR RST 2) ]
 *                                 │          └── Recuperado ─┐
 *                                 ▼ SÍ                       │
 *                                 ◄──────────────────────────┘
 *                                 ┌────────────────────────┐
 *                                 │ Desbloqueo Global HW   │
 *                                 │ (Write Enable + BP)    │
 *                                 └───────────┬────────────┘
 *                                             ▼
 *                            [5]  ¿Desbloqueo OK y sin TimeOut (BSY)?
 *                                 ├── NO ──► [ ABORTAR (ERR PASO 5) ]
 *                                 ▼ SÍ
 *                                 ┌────────────────────────┐
 *                                 │ Configurar Particiones │
 *                                 │ (LOAD, LOG, OTHER)     │
 *                                 └───────────┬────────────┘
 *                                             ▼
 *                            [6]  ¿Solapamientos o exceso de sectores?
 *                                 │
 *                                 ├── SÍ ──► [ ERROR: flash->init = false ]
 *                                 │
 *                                 ▼ NO
 *                                 [ ÉXITO: flash->init = true ]
 *                                             │
 *                                             ▼
 *                                 ┌────────────────────────┐
 *                                 │    FIN DEL PROCESO     │
 *                                 └────────────────────────┘
 * ==========================================================================================
 */


void Flash_Init(SPI_Flash_TypeDef *flash, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin, bool mensaje)
{
	char msn_buffer[250];
	uint32_t tick_inicio = HAL_GetTick();

	osDelay(TXFLASHLOG);
		MSG_Enviar(MSG_GENERAL,
		   "\r\n==========================================================\r\n"
		   " [SPI FLASH] [Flash_Init] INIT INIT_SPI\r\n"
		   "==========================================================\r\n",
		   Uart1
		);
	osDelay(TXFLASHLOG);

    // =========================================================================
    // 1. VERIFICACION AL CONSTRUCTOR DE LA FUNCION
    // =========================================================================
	if (flash == NULL || hspi == NULL || cs_port == NULL) {
	        snprintf(msn_buffer, sizeof(msn_buffer),
	                 "\r\n [SPI FLASH FATAL] [%s] [1/6] Abortado: Punteros NULL.\r\n"
	                 "  |- flash   : %p\r\n"
	                 "  |- hspi    : %p\r\n"
	                 "  \\- cs_port : %p (Pin: 0x%04X)\r\n",
	                 __func__, (void*)flash, (void*)hspi, (void*)cs_port, cs_pin);

	        osDelay(TXFLASHLOG);
	        MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
	        osDelay(TXFLASHLOG);

	        if (flash != NULL) flash->init = false;


	        osDelay(TXFLASHLOG);
	        MSG_Enviar(MSG_GENERAL,
	            "\r\n==========================================================\r\n"
	            " [SPI FLASH] [Flash_Init] FINAL INIT_SPI (ERR PASO 1)\r\n"
	            "==========================================================\r\n",
	            Uart1
	        );
	        osDelay(TXFLASHLOG);
	        return;
	    }


    // =========================================================================
    //2. CONSTRUCCION Y CREACION DE MUTEX
    // =========================================================================
    flash->hspi = hspi;
    flash->cs_port = cs_port;
    flash->cs_pin = cs_pin;
    flash->reset_init = FLASH_RESET_INIT;
    flash->reset_exec = FLASH_RESET_EXEC;
    flash->init = false;

    // =========================================================================
    // 2. CONSTRUCCION Y CREACION DE MUTEX
    // =========================================================================
    if (flash->spi_mutex == NULL) {
		flash->spi_mutex = osMutexNew(NULL);
		if (flash->spi_mutex == NULL) {

			snprintf(msn_buffer, sizeof(msn_buffer),
					 "\r\n [SPI FLASH ERROR] [%s] [2/6] Fallo al crear Mutex SPI (NULL).\r\n",
					 __func__);

			osDelay(TXFLASHLOG);
			MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
			osDelay(TXFLASHLOG);

			flash->init = false;


			osDelay(TXFLASHLOG);
			MSG_Enviar(MSG_GENERAL,
				"\r\n==========================================================\r\n"
				" [SPI FLASH] [Flash_Init] FINAL INIT_SPI (ERR MUTEX)\r\n"
				"==========================================================\r\n",
				Uart1
			);
			osDelay(TXFLASHLOG);
			return;
		}
	}

    snprintf(msn_buffer, sizeof(msn_buffer),
                     "\r\n [SPI FLASH] [%s] [1-2/6] Constructor y Mutex OK.\r\n"
                     "  |- SPI   : %p\r\n"
                     "  |- CS    : %p (Pin: 0x%04X)\r\n"
                     "  \\- Mutex : %p\r\n",
                     __func__, (void*)flash->hspi->Instance, (void*)flash->cs_port, flash->cs_pin, (void*)flash->spi_mutex);

	osDelay(TXFLASHLOG);
		MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
	osDelay(TXFLASHLOG);

	// =========================================================================
	// 3. RESET DE HARD POR SW (FUERZA SPI A MODO ESTANDAR Y LIMPIA REGISTROS)
	// =========================================================================

    osDelay(100);
    if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {

		snprintf(msn_buffer, sizeof(msn_buffer),
				 "\r\n [SPI FLASH ERROR] [%s] [3/6] Timeout Mutex en CMD_RESET_ENABLE (0x%02X).\r\n",
				 __func__, CMD_RESET_ENABLE);

		osDelay(TXFLASHLOG);
			MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
		osDelay(TXFLASHLOG);


		osDelay(TXFLASHLOG);
		MSG_Enviar(MSG_GENERAL,
			"\r\n==========================================================\r\n"
			" [SPI FLASH] [Flash_Init] FINAL INIT_SPI (ERR PASO 3)\r\n"
			"==========================================================\r\n",
			Uart1
		);
		osDelay(TXFLASHLOG);

		flash->init = false;
		return;
	}

    uint8_t cmd_crm_reset[2] = {0xFF, 0xFF};
    uint8_t cmd_wakeup       = FLASH_WAKE_UP;
    uint8_t cmd_reset_enable = CMD_RESET_ENABLE;
    uint8_t cmd_reset        = CMD_RESET_EXECUTE;
    HAL_StatusTypeDef statusWakeup;
    HAL_StatusTypeDef statusResetEnable;
    HAL_StatusTypeDef statusResetExecute;

    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
    HAL_SPI_Transmit(flash->hspi, (uint8_t *)cmd_crm_reset, 2, 100);
    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

    HAL_Delay(20);

    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
    statusWakeup = HAL_SPI_Transmit(flash->hspi, &cmd_wakeup, 2, 100);
    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

    HAL_Delay(20);

    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
    statusResetEnable = HAL_SPI_Transmit(flash->hspi, &cmd_reset_enable, 1, 100);
    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

    HAL_Delay(20);

    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
    statusResetExecute = HAL_SPI_Transmit(flash->hspi, &cmd_reset, 1, 100);
    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

    HAL_Delay(20);

    osMutexRelease(flash->spi_mutex);

    snprintf(msn_buffer, sizeof(msn_buffer),
			 "\r\n [SPI FLASH] [%s] [3/6] Trama WakeUp y Reset SW enviada.\r\n"
			 "  |- CMD_WAKEUP     (0x%02X) -> HAL: %d\r\n"
			 "  |- CMD_RESET_EN   (0x%02X) -> HAL: %d\r\n"
			 "  \\- CMD_RESET_EXEC (0x%02X) -> HAL: %d\r\n",
			 __func__, cmd_wakeup, statusWakeup, cmd_reset_enable, statusResetEnable, cmd_reset, statusResetExecute);

	osDelay(TXFLASHLOG);
	MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
	osDelay(TXFLASHLOG);


	if (statusResetEnable != HAL_OK || statusResetExecute != HAL_OK) {

	        snprintf(msn_buffer, sizeof(msn_buffer),
	                 "\r\n [SPI FLASH WARN] [%s] [3/6] Fallo Reset SW (En:%d, Ex:%d). Init_Reset_SPI(0x1)...\r\n",
	                 __func__, statusResetEnable, statusResetExecute);

	        osDelay(TXFLASHLOG);
	        MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
	        osDelay(TXFLASHLOG);

	        if (!Flash_Init_Reset_SPI(flash, 0x1)) {
	            flash->init = false;

	            osDelay(TXFLASHLOG);
	            MSG_Enviar(MSG_GENERAL,
	                "\r\n==========================================================\r\n"
	                " [SPI FLASH] [Flash_Init] FINAL INIT_SPI (ERR RST 0x1)\r\n"
	                "==========================================================\r\n",
	                Uart1
	            );
	            osDelay(TXFLASHLOG);
	            return;
	        }
	    }


	// =========================================================================
	// 4. DETECCION DE CHIP Y RESETEO SISTEMA SPI
	// =========================================================================
    osDelay(200);

    if (!Flash_Init_detect_chip(flash)) {
            snprintf(msn_buffer, sizeof(msn_buffer),
                     "\r\n [SPI FLASH WARN] [%s] [4/6] ID no detectado. Lanzando Init_Reset_SPI(0x2)...\r\n",
                     __func__);

            osDelay(TXFLASHLOG);
            MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
            osDelay(TXFLASHLOG);

            if (!Flash_Init_Reset_SPI(flash, 0x2)) {
                flash->init = false;

                osDelay(TXFLASHLOG);
                MSG_Enviar(MSG_GENERAL,
                    "\r\n==========================================================\r\n"
                    " [SPI FLASH] [Flash_Init] FINAL INIT_SPI (ERR RST 0x2)\r\n"
                    "==========================================================\r\n",
                    Uart1
                );
                osDelay(TXFLASHLOG);
                return;
            }
        }
    // =========================================================================
    // 5. DESBLOQUEO GLOBAL DE HARDWARE (Elimina proteccion de fabrica)
    // =========================================================================
    if (!Flash_Complement_Write_Enable(flash)) {
		if (mensaje) {

			snprintf(msn_buffer, sizeof(msn_buffer),
					 "\r\n [SPI FLASH ERROR] [%s] [5/6] Fallo en Write Enable (0x06) para Unprotect.\r\n",
					 __func__);

			osDelay(TXFLASHLOG);
			MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
			osDelay(TXFLASHLOG);
		}

		flash->init = false;


		osDelay(TXFLASHLOG);
		MSG_Enviar(MSG_GENERAL,
			"\r\n==========================================================\r\n"
			" [SPI FLASH] [Flash_Init] FINAL INIT_SPI (ERR WR_EN)\r\n"
			"==========================================================\r\n",
			Uart1
		);
		osDelay(TXFLASHLOG);
		return;
    }

    if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
		snprintf(msn_buffer, sizeof(msn_buffer),
				 "\r\n [SPI FLASH ERROR] [%s] [5/6] Timeout de Mutex en cmd_unprotect (0x01).\r\n",
				 __func__);

		osDelay(TXFLASHLOG);
		MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
		osDelay(TXFLASHLOG);

		osDelay(TXFLASHLOG);
		MSG_Enviar(MSG_GENERAL,
			"\r\n==========================================================\r\n"
			" [SPI FLASH] [Flash_Init] FINAL INIT_SPI (ERR PASO 5)\r\n"
			"==========================================================\r\n",
			Uart1
		);
		osDelay(TXFLASHLOG);

		flash->init = false;
		return;
	}

    uint8_t cmd_unprotect = FLASH_CMD_BP;
    uint8_t status_val[2] = {0x00, 0x00};

    HAL_StatusTypeDef statusUnprotectCmd;
	HAL_StatusTypeDef statusUnprotectVal;

	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
	statusUnprotectCmd = HAL_SPI_Transmit(flash->hspi, &cmd_unprotect, 1, 100);
	statusUnprotectVal = HAL_SPI_Transmit(flash->hspi, status_val, 2, 100);
	HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

	HAL_Delay(20);

	osMutexRelease(flash->spi_mutex);

	snprintf(msn_buffer, sizeof(msn_buffer),
			 "\r\n [SPI FLASH] [%s] [5/6] Desbloqueo Global enviado.\r\n"
			 "  |- CMD Write Status (0x%02X)  -> HAL: %d\r\n"
			 "  \\- Status Data (0x%02X, 0x%02X) -> HAL: %d\r\n",
			 __func__, cmd_unprotect, statusUnprotectCmd, status_val[0], status_val[1], statusUnprotectVal);

	osDelay(TXFLASHLOG);
	MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
	osDelay(TXFLASHLOG);

	if (!Flash_Comeplement_Wait_For_Ready(flash, 500)) {

		snprintf(msn_buffer, sizeof(msn_buffer),
				 "\r\n [SPI FLASH ERROR] [%s] [5/6] Timeout (500ms) esperando BSY en Unprotect.\r\n",
				 __func__);

		osDelay(TXFLASHLOG);
		MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
		osDelay(TXFLASHLOG);


		osDelay(TXFLASHLOG);
		MSG_Enviar(MSG_GENERAL,
			"\r\n==========================================================\r\n"
			" [SPI FLASH] [Flash_Init] FINAL INIT_SPI (ERR BSY)\r\n"
			"==========================================================\r\n",
			Uart1
		);
		osDelay(TXFLASHLOG);

		flash->init = false;
		return;
	}


    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);
    HAL_Delay(10);

    // =========================================================================
    // CONFIGURACION DE SECTORES
    // =========================================================================
    bool configFail = false;
    const char *failed_part_name = "";

    for (int i = 0; i < FLASH_SECTION_COUNT_END; i++) {
        flash->partitions[i].configured = false;
    }

    if (!configFail && !Flash_Init_Config_Partition(flash, FLASH_SECTION_LOAD, 0, 1, "LOAD")) {
        configFail = true;
        failed_part_name = "FLASH_SECTION_LOAD (Sectores 0 al 1)";
    }

    if (!configFail && !Flash_Init_Config_Partition(flash, FLASH_SECTION_LOG_MONITOR, 3, 1000, "MONITOR")) {
        configFail = true;
        failed_part_name = "FLASH_SECTION_LOG_MONITOR (Sectores 3 al 1000)";
    }

    if (!configFail && !Flash_Init_Config_Partition(flash, FLASH_SECTION_OTHER, 1002, 4095, "OTHER")) {
        configFail = true;
        failed_part_name = "FLASH_SECTION_OTHER (Sectores 1002 al 4095)";
    }

    if (mensaje)
    {
        if (!configFail)
        {

            // Bloque único con toda la información de inicialización sin delays excesivos en consola
        	snprintf(msn_buffer, sizeof(msn_buffer),
        	         "\r\n==================================\r\n"
        	         " SECTORS \r\n"
        	         "==================================\r\n"
        	         " - SPI      : %p\r\n"
        	         " - CS       : %p (0x%04X)\r\n"
        	         " - Capacidad: %lu MB (%lu B)\r\n"
        	         " - Pagina   : %d B\r\n"
        	         " - Sectores : %d\r\n"
        	         " \\-- Mapa:\r\n",

        	         (void*)flash->hspi->Instance,
        	         (void*)flash->cs_port,
        	         flash->cs_pin,
        	         (unsigned long)GET_FLASH_CAPCITY,
        	         (unsigned long)GET_FLASH_CAPCITY,
        	         FLASH_PAGE_SIZE,
        	         FLASH_num_sector_count);

        	osDelay(TXFLASHLOG);
            MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
            osDelay(TXFLASHLOG);

            // Bucle de particiones optimizado
            for (int i = 0; i < FLASH_SECTION_COUNT_END; i++)
            {
                if (flash->partitions[i].configured)
                {
                    uint32_t start = flash->partitions[i].num_start_sector;
                    uint32_t end   = flash->partitions[i].num_end_sector;
                    uint32_t count = flash->partitions[i].num_sector_count;

                    snprintf(msn_buffer, sizeof(msn_buffer),
                             "    - Part. [%d]: %lu Sect. | S: %lu -> %lu\r\n",
                             i, (unsigned long)count, (unsigned long)start, (unsigned long)end);

                    osDelay(TXFLASHLOG);
                    MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
                    osDelay(TXFLASHLOG);

                    start = flash->partitions[i].start_sector;
				    end   = flash->partitions[i].end_sector;

				   snprintf(msn_buffer, sizeof(msn_buffer),
							"    - Part. [%d]: dirArrStar: %lu | dirArrEnd: %lu\r\n",
							i, (unsigned long)start, (unsigned long)end);

				   osDelay(TXFLASHLOG);
				   MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
				   osDelay(TXFLASHLOG);
                }
            }

            // Cierre visual
            snprintf(msn_buffer, sizeof(msn_buffer),
                     "    - Fin del Mapa de Memoria\r\n"
                     "==================================\r\n\r\n");
            osDelay(TXFLASHLOG);
			MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
			osDelay(TXFLASHLOG);
        }
        else
        {
			snprintf(msn_buffer, sizeof(msn_buffer),
					 "\r\n [SPI FLASH] [%.10s] [6/6] ERROR\r\n"
					 "  |- Mapeo  : %.26s\r\n"
					 "  |- Limite : %d Sectores Max.\r\n"
					 "  \\- Motivo : Limites excedidos.\r\n\r\n",
					 __func__,
					 failed_part_name,
					 FLASH_num_sector_count);

			osDelay(TXFLASHLOG);
			MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
			osDelay(TXFLASHLOG);
        }
    }

    if(!configFail) flash->init = true;

    snprintf(msn_buffer, sizeof(msn_buffer),
             "\r\n==============================\r\n"
             " [SPI FLASH] [%.10s] INIT\r\n"
             "  |- Estado : %s\r\n"
             "  |- Part.  : %.24s\r\n"
             "  \\- Tiempo : %lu ms\r\n"
             "==============================\r\n",
             __func__,
             flash->init ? "ONLINE (OK)" : "OFFLINE (ERROR)",
             failed_part_name,
             (unsigned long)(HAL_GetTick() - tick_inicio));

	osDelay(TXFLASHLOG);
	MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
	osDelay(TXFLASHLOG);

}

bool Flash_Init_Config_Partition(SPI_Flash_TypeDef *flash, Flash_Partition_ID part_id, uint32_t start_sector, uint32_t end_sector, const char *namePartition)
{

    char msn_buffer[256];

    if (flash == NULL) {
        snprintf(msn_buffer, sizeof(msn_buffer),
                 "\r\n [SPI FLASH ERROR] [%s] Puntero nulo detectado en el objeto flash\r\n", __func__);
        MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
        return false;
    }

    if (part_id >= FLASH_SECTION_COUNT_END) {
        snprintf(msn_buffer, sizeof(msn_buffer),
                 "\r\n [SPI FLASH ERROR] [%s] ID de particion invalido o fuera de rango (ID: %d)\r\n", __func__, part_id);
        MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
        return false;
    }

    if (end_sector < start_sector) {
        snprintf(msn_buffer, sizeof(msn_buffer),
                 "\r\n [SPI FLASH ERROR] [%s] Rango ilogico en Part. [%lu] (Inicio: %lu > Fin: %lu)\r\n", __func__, (unsigned long)part_id, (unsigned long)start_sector, (unsigned long)end_sector);
        MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
        return false;
    }

    if (end_sector >= FLASH_num_sector_count) {
        snprintf(msn_buffer, sizeof(msn_buffer),
                 "\r\n [SPI FLASH ERROR] [%s] La Part. [%lu] excede el limite fisico (Fin: %lu / Max: %d)\r\n", __func__, (unsigned long)part_id, (unsigned long)end_sector, FLASH_num_sector_count);
        MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
        return false;
    }

    if (strlen(namePartition) > (STRUCT_NAME_PARTITION - 1)) {
        snprintf(msn_buffer, sizeof(msn_buffer),
                 "\r\n [SPI FLASH ERROR] [%s] Error longitud del nombre de la particion [%s]\r\n", __func__, namePartition);
        MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
        return false;
    }

    if (!Flash_Init_Check_Partition_Overlap(flash, part_id, start_sector, end_sector)) {
        return false;
    }

    // 3. CONFIGURACIÓN DE LA PARTICIÓN
    Flash_Partition_TypeDef *part = &flash->partitions[part_id];

    memset(part, 0, sizeof(Flash_Partition_TypeDef));

    part->id             = part_id;
    part->configured     = true;
    flash->init          = false;
    flash->run           = false;

    strncpy(part->name, namePartition, STRUCT_NAME_PARTITION - 1);
    part->name[STRUCT_NAME_PARTITION - 1] = '\0';

    part->num_start_sector   = start_sector;
    part->num_end_sector     = end_sector;
    part->num_sector_count   = ((end_sector - start_sector) + 1);
    part->start_sector       = start_sector * FLASH_SECTION_SIZE;
    part->end_sector         = end_sector * FLASH_SECTION_SIZE;
    part->current_sector     = start_sector;
    part->sector_offset      = 0;


    switch (part_id) {
		case FLASH_SECTION_LOG_MONITOR:
			part->sectionMonitor.monitor.contador = 0;
			break;

		case FLASH_SECTION_LOAD:
			break;

		case FLASH_SECTION_OTHER:
			break;

		default:
			break;
        }

    return true;
}

bool Flash_Init_Check_Partition_Overlap(SPI_Flash_TypeDef *flash, Flash_Partition_ID part_id, uint32_t start_sector, uint32_t end_sector)
{
    // Límite estricto de 250 bytes para el buffer
    char msn_buffer[250];

    for (int i = 0; i < FLASH_SECTION_COUNT_END; i++)
    {
        if (i != part_id && flash->partitions[i].configured)
        {
            uint32_t other_start = flash->partitions[i].num_start_sector;
            uint32_t other_end   = flash->partitions[i].num_end_sector;

            // La lógica de solapamiento es correcta
            if ((start_sector <= other_end) && (other_start <= end_sector))
            {
                // Mensaje optimizado para no superar los ~120 caracteres en total
                // Uso de %lu y cast a (unsigned long) para variables uint32_t
                snprintf(msn_buffer, sizeof(msn_buffer),
                         "\r\n[FLASH ERROR] %s: Solapamiento!\r\n"
                         "├─ Intento Part. [%d] (%lu -> %lu)\r\n"
                         "└─ Choca c/Part. [%d] (%lu -> %lu)\r\n",
                         __func__,
                         part_id, (unsigned long)start_sector, (unsigned long)end_sector,
                         i, (unsigned long)other_start, (unsigned long)other_end);

                // Opcional: solo un delay corto si hay ráfagas previas en la UART
                // osDelay(TXFLASHLOG);

                MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);

                // ELIMINADO: osDelay(TXFLASHLOG) después del envío.
                // MSG_Enviar debe gestionar su propia bandera de fin de transmisión (TxCpltCallback)

                return false;
            }
        }
    }
    return true;
}

bool Flash_Init_detect_chip(SPI_Flash_TypeDef *flash)
{
    if (flash == NULL) {return false;}

    uint8_t cmd = FLASH_CMD_ID;
    uint8_t id_rx[3] = {0};
    char msn_buffer[256];


    if (osMutexAcquire(flash->spi_mutex, 500) != osOK) {
        snprintf(msn_buffer, sizeof(msn_buffer),
                 "\r\n [SPI FLASH ERROR] [%s] Timeout de Mutex: El bus SPI esta ocupado.\r\n", __func__);
        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
        osDelay(TXFLASHLOG);
        return false;
    }


    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);

    HAL_StatusTypeDef spi_status = HAL_SPI_Transmit(flash->hspi, &cmd, 1, WRITE_READ_NORMAL_VELOCITY);

    if (spi_status == HAL_OK) {
        spi_status = HAL_SPI_Receive(flash->hspi, id_rx, 3, WRITE_READ_NORMAL_VELOCITY);
    }

    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

    osMutexRelease(flash->spi_mutex);


    if (spi_status != HAL_OK) {
        snprintf(msn_buffer, sizeof(msn_buffer),
                 "\r\n [SPI FLASH ERROR] [%s] HAL_SPI ha devuelto un error (Code: %d).\r\n",
                 __func__, spi_status);
        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
        osDelay(TXFLASHLOG);
        return false;
    }


    if (id_rx[0] == 0x00 || id_rx[2] == 0x00) {
        snprintf(msn_buffer, sizeof(msn_buffer),
                 "\r\n [SPI FLASH ERROR] [%s] Fallo comunicacion SPI.\r\n"
                 " └─ ID: 0x00 (Posible corto a GND o chip danado).\r\n\r\n", __func__);
        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
        osDelay(TXFLASHLOG);
        return false;
    }

    if (id_rx[0] == 0xFF || id_rx[2] == 0xFF) {
        snprintf(msn_buffer, sizeof(msn_buffer),
                 "\r\n [SPI FLASH ERROR] [%s] Fallo comunicacion SPI.\r\n"
                 " └─ ID: 0xFF (Chip ausente o pines desoldados).\r\n\r\n", __func__);
        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
        osDelay(TXFLASHLOG);
        return false;
    }


    const char *fabricante = NULL;
    const char *capacidad  = NULL;

    switch (id_rx[0]) {
        case 0xC8: fabricante = "GigaDevice (GD25Q...)"; break;
        case 0xEF: fabricante = "Winbond (W25Q...)";     break;
        case 0xC2: fabricante = "Macronix (MX25L...)";   break;
        default:   fabricante = "Desconocido";           break;
    }


    switch (id_rx[2]) {
        case 0x18: capacidad = "16 MB (128 Mbit)"; break;
        case 0x17: capacidad = "8 MB (64 Mbit)";   break;
        default:   capacidad = "Desconocida";      break;
    }

    bool chip_valido = (id_rx[0] == 0xC8 || id_rx[0] == 0xEF || id_rx[0] == 0xC2) &&
                       (id_rx[2] == 0x18 || id_rx[2] == 0x17);


    snprintf(msn_buffer, sizeof(msn_buffer),
             "\r\n [SPI FLASH] [%s] Analisis de Hardware %s:\r\n"
             " ├─ JEDEC ID Raw : %02X %02X %02X\r\n"
             " ├─ Fabricante   : %s\r\n"
             " └─ Capacidad    : %s\r\n",
             __func__,
             chip_valido ? "Exitoso" : "No Soportado",
             id_rx[0], id_rx[1], id_rx[2],
             fabricante,
             capacidad);

    if (!MSG_Enviar_Longitud_Limit(msn_buffer)) {
        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,msn_buffer, Uart1);
        osDelay(TXFLASHLOG);
    }

    return chip_valido;
}

bool Flash_Hardware_Double_Check(SPI_Flash_TypeDef *flash, bool mensaje)
{
    if (flash == NULL || flash->hspi == NULL) return false;

    char msg[256];
    uint8_t status_reg = 0xFF;
    uint8_t cmd_read_status = FLASH_CMD_READ_STATUS;
    uint8_t cmd_jedec = FLASH_CMD_ID;
    uint8_t jedec_id[3] = {0, 0, 0};

    if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
        if (mensaje) {
            snprintf(msg, sizeof(msg), "\r\n [SPI FLASH HW FAIL] [%s] Timeout de Mutex.\r\n", __func__);
            osDelay(TXFLASHLOG);
            MSG_Enviar(MSG_GENERAL,msg, Uart1);
            osDelay(TXFLASHLOG);
        }
        return false;
    }

    // =========================================================
    // TEST 1: Comprobacion del Registro de Estado (Bit WIP/BUSY)
    // =========================================================
    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef st1 = HAL_SPI_Transmit(flash->hspi, &cmd_read_status, 1, 100);
    if (st1 == HAL_OK) {
        st1 = HAL_SPI_Receive(flash->hspi, &status_reg, 1, 100);
    }
    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

    // =========================================================
    // TEST 2: Comprobacion de Integridad del Bus (Re-lectura JEDEC ID)
    // =========================================================
    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
    HAL_StatusTypeDef st2 = HAL_SPI_Transmit(flash->hspi, &cmd_jedec, 1, 100);
    if (st2 == HAL_OK) {
        st2 = HAL_SPI_Receive(flash->hspi, jedec_id, 3, 100);
    }
    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

    osMutexRelease(flash->spi_mutex);

    if (st1 != HAL_OK || st2 != HAL_OK) {
        if (mensaje) {
            snprintf(msg, sizeof(msg), "\r\n [SPI FLASH HW FAIL] [%s] Error HAL en Double Check (%d, %d).\r\n",
                     __func__, st1, st2);
            osDelay(TXFLASHLOG);
            MSG_Enviar(MSG_GENERAL,msg, Uart1);
            osDelay(TXFLASHLOG);
        }
        return false;
    }


    if ((status_reg & 0x01) != 0) {
        if (mensaje) {
            snprintf(msg, sizeof(msg),
                     "\r\n [SPI FLASH HW FAIL] [%s] Chip atascado en estado BUSY (SR1: 0x%02X).\r\n",
                     __func__, status_reg);
            osDelay(TXFLASHLOG);
            MSG_Enviar(MSG_GENERAL,msg, Uart1);
            osDelay(TXFLASHLOG);
        }
        return false;
    }


    if ((jedec_id[0] == 0x00 || jedec_id[2] == 0x00) ||
        (jedec_id[0] == 0xFF || jedec_id[2] == 0xFF)) {
        if (mensaje) {
            snprintf(msg, sizeof(msg),
                     "\r\n [SPI FLASH HW FAIL] [%s] Bus MISO inestable. JEDEC: %02X %02X %02X\r\n",
                     __func__, jedec_id[0], jedec_id[1], jedec_id[2]);
            osDelay(TXFLASHLOG);
            MSG_Enviar(MSG_GENERAL,msg, Uart1);
            osDelay(TXFLASHLOG);
        }
        return false;
    }

    if (mensaje) {

        snprintf(msg, sizeof(msg),
                 "\r\n [SPI FLASH] [%s] HW Double Check OK (SR1: 0x%02X | JEDEC: %02X%02X%02X)\r\n",
                 __func__, status_reg, jedec_id[0], jedec_id[1], jedec_id[2]);
        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,msg, Uart1);
        osDelay(TXFLASHLOG);
    }

    return true;
}

bool Flash_Init_Reset_SPI(SPI_Flash_TypeDef *flash, uint32_t codePositionReset)
{

    if (flash == NULL || flash->hspi == NULL) return false;

    char msg[256];

    snprintf(msg, sizeof(msg),
		"\r\n==========================================================\r\n"
		" [SPI FLASH] [Flash_Reset_SPI] INIT RESET SPI %lu\r\n"
		"==========================================================\r\n",
		(unsigned long)codePositionReset
	);

    osDelay(TXFLASHLOG);
    MSG_Enviar(MSG_GENERAL,msg, Uart1);
    osDelay(TXFLASHLOG);

    // =========================================================================
    // DIAGNOSTICO DEL ERROR DEL HARDWARE (Lectura de máscara de bits)
    // =========================================================================
    uint32_t codigo_error = HAL_SPI_GetError(flash->hspi);

    if (codigo_error != HAL_SPI_ERROR_NONE) {
        if (codigo_error & HAL_SPI_ERROR_OVR) {
            snprintf(msg, sizeof(msg), "\r\n [SPI FLASH HW FAIL] Overrun (OVR): RX FIFO saturada. Cod: 0x%08lX\r\n", (unsigned long)codigo_error);
        }
        else if (codigo_error & HAL_SPI_ERROR_UDR) {
            snprintf(msg, sizeof(msg), "\r\n [SPI FLASH HW FAIL] Underrun (UDR): TX FIFO vacia. Cod: 0x%08lX\r\n", (unsigned long)codigo_error);
        }
        else if (codigo_error & HAL_SPI_ERROR_MODF) {
            snprintf(msg, sizeof(msg), "\r\n [SPI FLASH HW FAIL] Mode Fault (MODF): Conflicto en pin CS. Cod: 0x%08lX\r\n", (unsigned long)codigo_error);
        }
        else if (codigo_error & HAL_SPI_ERROR_FRE) {
            snprintf(msg, sizeof(msg), "\r\n [SPI FLASH HW FAIL] Frame Error (FRE): Desincronizacion SCK. Cod: 0x%08lX\r\n", (unsigned long)codigo_error);
        }
        else if (codigo_error & HAL_SPI_ERROR_CRC) {
            snprintf(msg, sizeof(msg), "\r\n [SPI FLASH HW FAIL] CRC Error: Fallo de verificacion. Cod: 0x%08lX\r\n", (unsigned long)codigo_error);
        }
        else if (codigo_error & HAL_SPI_ERROR_DMA) {
            snprintf(msg, sizeof(msg), "\r\n [SPI FLASH HW FAIL] DMA Error: Colapso en controlador DMA. Cod: 0x%08lX\r\n", (unsigned long)codigo_error);
        }
        else if (codigo_error & HAL_SPI_ERROR_FLAG) {
            snprintf(msg, sizeof(msg), "\r\n [SPI FLASH HW FAIL] Flag Error: Error interno motor SPI. Cod: 0x%08lX\r\n", (unsigned long)codigo_error);
        }
        else if (codigo_error & HAL_SPI_ERROR_ABORT) {
            snprintf(msg, sizeof(msg), "\r\n [SPI FLASH HW FAIL] Abort Error: Fallo al detener bus. Cod: 0x%08lX\r\n", (unsigned long)codigo_error);
        }
        else {
            snprintf(msg, sizeof(msg), "\r\n [SPI FLASH HW FAIL] Fallo critico desconocido. Cod: 0x%08lX\r\n", (unsigned long)codigo_error);
        }

        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,msg, Uart1);
        osDelay(TXFLASHLOG);
    }

    // =========================================================================
    // BLOQUEO EXCLUSIVO DEL BUS SPI (FreeRTOS Mutex)
    // =========================================================================
    if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH ERROR] Timeout de Mutex en intento de reset.\r\n", Uart1);
        osDelay(TXFLASHLOG);

        MSG_Enviar(MSG_GENERAL,
            "\r\n==========================================================\r\n"
            " [SPI FLASH] [Flash_Reset_SPI] FIN RESET SPI (FALLIDO)\r\n"
            "==========================================================\r\n",
            Uart1
        );
        return false;
    }

    // Asegurar físicamente que la memoria deja de escuchar por si el CS se quedó bajo
    HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);
    osDelay(5);

    // =========================================================================
    // BUCLE DE REINTENTOS DE REINICIO FÍSICO DEL MICROCONTROLADOR (STM32)
    // =========================================================================
    bool statusReset = false;

    for (short i = 0;  i < FLASH_RESET_INIT; ++i) {

        snprintf(msg, sizeof(msg), "\r\n [SPI FLASH] Ejecutando Reset HW (Intento %d/%d)...\r\n", i + 1, FLASH_RESET_INIT);
        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,msg, Uart1);
        osDelay(TXFLASHLOG);

        // Apagado Lógico
        (void)HAL_SPI_Abort(flash->hspi);
        HAL_SPI_DeInit(flash->hspi);
        osDelay(10);

        // Choque eléctrico (RCC Reset)
        if (flash->hspi->Instance == SPI1) {
            __HAL_RCC_SPI1_FORCE_RESET();
            __HAL_RCC_SPI1_RELEASE_RESET();
        }
        else if (flash->hspi->Instance == SPI2) {
            __HAL_RCC_SPI2_FORCE_RESET();
            __HAL_RCC_SPI2_RELEASE_RESET();
        }
        else if (flash->hspi->Instance == SPI3) {
            __HAL_RCC_SPI3_FORCE_RESET();
            __HAL_RCC_SPI3_RELEASE_RESET();
        }
        else if (flash->hspi->Instance == SPI4) {
            __HAL_RCC_SPI4_FORCE_RESET();
            __HAL_RCC_SPI4_RELEASE_RESET();
        }
        else if (flash->hspi->Instance == SPI5) {
            __HAL_RCC_SPI5_FORCE_RESET();
            __HAL_RCC_SPI5_RELEASE_RESET();
        }
        else if (flash->hspi->Instance == SPI6) {
            __HAL_RCC_SPI6_FORCE_RESET();
            __HAL_RCC_SPI6_RELEASE_RESET();
        }

        osDelay(10);


        if (HAL_SPI_Init(flash->hspi) == HAL_OK) {
        	__HAL_SPI_CLEAR_OVRFLAG(flash->hspi);

			while (__HAL_SPI_GET_FLAG(flash->hspi, SPI_FLAG_RXNE)) {
				volatile uint8_t basura_fantasma = *(volatile uint8_t *)&flash->hspi->Instance->RXDR;
				(void)basura_fantasma;
			}

			snprintf(msg, sizeof(msg), " [SPI FLASH] OK: Bus SPI recuperado con exito en el intento %d.\r\n", i + 1);
			osDelay(TXFLASHLOG);
			MSG_Enviar(MSG_GENERAL,msg, Uart1);
			osDelay(TXFLASHLOG);

			statusReset = true;
			break;
        }

        snprintf(msg, sizeof(msg), " [SPI FLASH] WARN: Intento %d fallido. Preparando nuevo ciclo...\r\n", i + 1);
        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,msg, Uart1);
        osDelay(TXFLASHLOG);

    } // FIN BUCLE FOR

    if (!statusReset) {
        osDelay(TXFLASHLOG);
        MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH] ERROR FATAL: No se pudo reiniciar el SPI tras los reintentos.\r\n", Uart1);
        osDelay(TXFLASHLOG);
    }

    // =========================================================================
    //RESETEO VIA SOFTWARE DEL CHIP ESCLAVO
    // =========================================================================
    if (statusReset) {
		uint8_t cmd_crm_reset[2] = {0xFF, 0xFF};
		uint8_t cmd_wakeup       = 0xAB;
		uint8_t cmd_reset_enable = CMD_RESET_ENABLE;
		uint8_t cmd_reset        = CMD_RESET_EXECUTE;

		// 1. Romper posible modo continuo y despertar de Deep Power-Down
		HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
		HAL_SPI_Transmit(flash->hspi, cmd_crm_reset, 2, 100);
		HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);
		osDelay(2);

		HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
		HAL_SPI_Transmit(flash->hspi, &cmd_wakeup, 1, 100);
		HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);
		osDelay(5);

		// 2. Habilitar y ejecutar reseteo por software (0x66 + 0x99)
		HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
		HAL_StatusTypeDef st_en = HAL_SPI_Transmit(flash->hspi, &cmd_reset_enable, 1, 100);
		HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

		osDelay(10);

		HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_RESET);
		HAL_StatusTypeDef st_ex = HAL_SPI_Transmit(flash->hspi, &cmd_reset, 1, 100);
		HAL_GPIO_WritePin(flash->cs_port, flash->cs_pin, GPIO_PIN_SET);

		osDelay(50); // Tiempo para reinicio interno de la memoria Flash

		if (st_en != HAL_OK || st_ex != HAL_OK) {
			statusReset = false;
		}
	}

    osMutexRelease(flash->spi_mutex);

    if (statusReset) {
            if (!Flash_Hardware_Double_Check(flash, true)) {
			statusReset = false;
		}
	}

	// [180 / 256 bytes]
	snprintf(msg, sizeof(msg),
		"\r\n==========================================================\r\n"
		" [SPI FLASH] [Flash_Reset_SPI] FIN RESET SPI %lu (%s)\r\n"
		"==========================================================\r\n",
		(unsigned long)codePositionReset,
		statusReset ? "RECUPERADO" : "FALLIDO"
	);

	osDelay(TXFLASHLOG);
	MSG_Enviar(MSG_GENERAL,msg, Uart1);
	osDelay(TXFLASHLOG);

	return statusReset;
}

