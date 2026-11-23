#include <string.h>
#include <stdio.h>

#include "cmsis_os.h"


#include "SPI_Flash_Memory/SPI_Flash_Sections/SPI_flash_section_log_monitor/SPI_Flash_Logs_Monitor.h"
#include "MsgUART/TxUart/TxUart.h"


extern I_TestUart Uart1;

bool Flash_Monitor_Init(SPI_Flash_TypeDef *flash){

	//revisar el centinal tiene que se el que está vacio
	osDelay(TXFLASHLOG);
	   MSG_Enviar(MSG_GENERAL,
		   "\r\n==========================================================\r\n"
		   " [SPI FLASH] [Flash_Monitor_Init] INIT MONITOR_SPI\r\n"
		   "==========================================================\r\n",
		   Uart1
	   );
	   osDelay(TXFLASHLOG);

	// =========================================================================
	// VERIFICACIONES
	// =========================================================================
	   flash->partitions[FLASH_SECTION_LOG_MONITOR].run = false;
	   flash->partitions[FLASH_SECTION_LOG_MONITOR].init = false;

	//PUNTERO -->
	if (flash == NULL) {
		osDelay(TXFLASHLOG);
		   MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH FATAL] [Flash_Monitor_Init] Inicializacion abortada: Referencias de hardware nulas (NULL).\r\n", Uart1);

		   MSG_Enviar(MSG_GENERAL,
			   "\r\n==========================================================\r\n"
			   " [SPI FLASH] [Flash_Monitor_Init] FINAL MONITOR_SPI\r\n"
			   "==========================================================\r\n",
			   Uart1
		   );
		   osDelay(TXFLASHLOG);
	return false;
	}


	//INICIALIZACIÓN FLASH -->
	if (!flash->init || !flash->load || !flash->partitions[FLASH_SECTION_LOAD].run) {
		osDelay(TXFLASHLOG);
		   MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH FATAL] [Flash_Monitor_Init] Inicializacion abortada: SPI o Load no iniciado.\r\n", Uart1);
		osDelay(TXFLASHLOG);
		   MSG_Enviar(MSG_GENERAL,
			   "\r\n==========================================================\r\n"
			   " [SPI FLASH] [Flash_Monitor_Init] FINAL MONITOR_SPI\r\n"
			   "==========================================================\r\n",
			   Uart1
		   );
		   osDelay(TXFLASHLOG);

		   flash->partitions[FLASH_SECTION_LOG_MONITOR].run = false;
		   flash->partitions[FLASH_SECTION_LOG_MONITOR].init = false;
	return false;
	}

	//PARTICION SPI -->
	bool statusFindSection = false;
	bool statusSize = false;

	for (int i = 0; i < FLASH_SECTION_COUNT_END; ++i) {
		if(flash->partitions[i].id == FLASH_SECTION_LOG_MONITOR){
			statusFindSection = true;

			if (flash->partitions[i].num_sector_count > NUMBER_SECTOR_MINIMUN) {
				statusSize = true;
			}
			break;
		}
	}

	if (!statusFindSection || !statusSize) {
			osDelay(TXFLASHLOG);

			if (!statusFindSection) {
				osDelay(TXFLASHLOG);
				MSG_Enviar(MSG_GENERAL,
					"\r\n [SPI FLASH FATAL] [Flash_Monitor_Init] Inicializacion abortada: Particion LOG_MONITOR no encontrada.\r\n",
					Uart1
				);
				osDelay(TXFLASHLOG);
			} else {
				osDelay(TXFLASHLOG);
				MSG_Enviar(MSG_GENERAL,
					"\r\n [SPI FLASH FATAL] [Flash_Monitor_Init] Inicializacion abortada: Sectores insuficientes en LOG_MONITOR.\r\n",
					Uart1
				);
				osDelay(TXFLASHLOG);
			}

		osDelay(TXFLASHLOG);
		MSG_Enviar(MSG_GENERAL,
			"\r\n==========================================================\r\n"
			" [SPI FLASH] [Flash_Monitor_Init] FINAL MONITOR_SPI\r\n"
			"==========================================================\r\n",
			Uart1
		);
		osDelay(TXFLASHLOG);

		flash->partitions[FLASH_SECTION_LOG_MONITOR].run = false;
		flash->partitions[FLASH_SECTION_LOG_MONITOR].init = false;

		return false;
	}

	// =========================================================================
	// SECTOR CENTINELA
	// =========================================================================

	bool findCentinela = false;
	uint32_t addrCentinela = 0;
	uint32_t contador = 0;
	uint32_t centinela = FLASH_MAGIC_CENTINELA;
	uint32_t dataSearch [2] = {0,0};
	bool     statusFlash = true;
	bool     protectionLimit = false;
	char     buf_msg[256];

	uint32_t start_addr = flash->partitions[FLASH_SECTION_LOG_MONITOR].start_sector;
	uint32_t end_addr   = flash->partitions[FLASH_SECTION_LOG_MONITOR].end_sector;

	 if (osMutexAcquire(flash->spi_mutex, osWaitForever) != osOK) {
	  osDelay(TXFLASHLOG);
	  MSG_Enviar(MSG_GENERAL,"\r\n [SPI FLASH OCUPED][Flash_read_One_Sector_Search] SPI secuestrado.\r\n", Uart1);
	  osDelay(TXFLASHLOG);
	  return false;
	}

    // BUSQUEDAD -->
	 for (uint32_t addrFor = start_addr; addrFor <= end_addr; addrFor += FLASH_SECTION_SIZE) {

		 statusFlash = Flash_read_One_Sector_Search_No_Mutex(flash, addrFor, sizeof(dataSearch), (uint8_t*)&dataSearch);

		 if (!statusFlash) {
				snprintf(buf_msg, sizeof(buf_msg),
						"\r\n [SPI FLASH FATAL] [Flash_Monitor_Init] Inicializacion abortada: fallo de lectura hardware buscando el Centinela en Dir: 0x%06lX. Abortando.\r\n",
					(unsigned long)addrFor);

				osDelay(TXFLASHLOG);
				MSG_Enviar(MSG_GENERAL,buf_msg, Uart1);
				osDelay(TXFLASHLOG);



				osDelay(TXFLASHLOG);
				MSG_Enviar(MSG_GENERAL,
					"\r\n==========================================================\r\n"
					" [SPI FLASH] [Flash_Monitor_Init] FINAL MONITOR_SPI\r\n"
					"==========================================================\r\n",
					Uart1
				);
				osDelay(TXFLASHLOG);

				osMutexRelease(flash->spi_mutex);

				return false;
		}



		uint32_t leido_centinela = dataSearch[0];
		uint32_t leido_contador  = dataSearch[1];

		if (FLASH_DEBUG_FLASH) {
			snprintf(buf_msg, sizeof(buf_msg), "  |- Dir: 0x%06lX | Cent: 0x%08lX | Cont: %lu\r\n",
					 (unsigned long)addrFor, (unsigned long)leido_centinela, (unsigned long)leido_contador);
			osDelay(TXFLASHLOG);
			MSG_Enviar(MSG_GENERAL,buf_msg, Uart1);
			osDelay(TXFLASHLOG);
		}

		// Comparamos EXCLUSIVAMENTE el centinela
		if (leido_centinela == centinela)
		{
			findCentinela = true;
			addrCentinela = addrFor;
			break;
		}
	}


	osMutexRelease(flash->spi_mutex);

	// ASIGNACION CONTADOR -->
	if(findCentinela){
		osDelay(TXFLASHLOG);
		MSG_Enviar(MSG_GENERAL,
			"\r\n [SPI FLASH FATAL] [Flash_Monitor_Init] Buscando centinela en sección anterior.\r\n",
			Uart1
		);
		osDelay(TXFLASHLOG);

		uint32_t addrCentinelaPrev = addrCentinela - FLASH_SECTION_SIZE;

		Flash_read_One_Sector_Search(flash, addrCentinelaPrev , sizeof(dataSearch), (uint8_t*)&dataSearch);

		contador = dataSearch[1];

		if(contador >= 4294967295){
			osDelay(TXFLASHLOG);
			MSG_Enviar(MSG_GENERAL,
				"\r\n [SPI FLASH FATAL] [Flash_Monitor_Init] El contador a fallao sistema abortado.\r\n",
				Uart1
			);
			osDelay(TXFLASHLOG);
			osDelay(TXFLASHLOG);
			MSG_Enviar(MSG_GENERAL,
				"\r\n==========================================================\r\n"
				" [SPI FLASH] [Flash_Monitor_Init] FINAL MONITOR_SPI\r\n"
				"==========================================================\r\n",
				Uart1
			);
			osDelay(TXFLASHLOG);

			return false;
		}

	}



	if (findCentinela) {

		snprintf(buf_msg, sizeof(buf_msg),
				 "\r\n [SPI FLASH OK] Centinela encontrado.\r\n"
				 "  \\- Direccion : 0x%06lX\r\n"
				 "  \\- Contador : %lu\r\n",
				 (unsigned long)addrCentinela, (unsigned long)contador);
		osDelay(TXFLASHLOG);
		MSG_Enviar(MSG_GENERAL,buf_msg, Uart1);
		osDelay(TXFLASHLOG);
	} else {
		snprintf(buf_msg, sizeof(buf_msg),
				 "\r\n [SPI FLASH WARN] Fin de busqueda. Centinela NO encontrado.\r\n");
		osDelay(TXFLASHLOG);
		MSG_Enviar(MSG_GENERAL,buf_msg, Uart1);
		osDelay(TXFLASHLOG);
	}







   //NO ENCONTRAMOS CENTINELA BORRAMOS E INICALIZAMOS -->
	if (!findCentinela) {


		//INICIALIZACION EN POSICION 0 -->

	   protectionLimit = Flas_Monitor_protetionLimit_Range(flash, start_addr, end_addr);

		if (!protectionLimit) {
			osDelay(TXFLASHLOG);
			MSG_Enviar(MSG_GENERAL,
				"\r\n [SPI FLASH FATAL] [Flash_Monitor_Init] Inicializacion abortada: fallo en limites de sectores.\r\n",
				Uart1
			);
			osDelay(TXFLASHLOG);
			osDelay(TXFLASHLOG);
			MSG_Enviar(MSG_GENERAL,
				"\r\n==========================================================\r\n"
				" [SPI FLASH] [Flash_Monitor_Init] FINAL MONITOR_SPI\r\n"
				"==========================================================\r\n",
				Uart1
			);
			osDelay(TXFLASHLOG);


			return false;
		}


		statusFlash = Flash_Erase_One_Hex_For_Sector(flash, start_addr);

		if (!statusFlash) {
			osDelay(TXFLASHLOG);
			MSG_Enviar(MSG_GENERAL,
				"\r\n [SPI FLASH FATAL] [Flash_Monitor_Init] Inicializacion abortada: fallo en borrado de sectores.\r\n",
				Uart1
			);
			osDelay(TXFLASHLOG);
			osDelay(TXFLASHLOG);
			MSG_Enviar(MSG_GENERAL,
				"\r\n==========================================================\r\n"
				" [SPI FLASH] [Flash_Monitor_Init] FINAL MONITOR_SPI\r\n"
				"==========================================================\r\n",
				Uart1
			);
			osDelay(TXFLASHLOG);


			return false;
		}

		flash->partitions[FLASH_SECTION_LOG_MONITOR].current_sector = 0;
		flash->partitions[FLASH_SECTION_LOG_MONITOR].current_page = 0;
		flash->partitions[FLASH_SECTION_LOG_MONITOR].sector_offset = 0;
		flash->partitions[FLASH_SECTION_LOG_MONITOR].sectionMonitor.monitor.contador = 0;

		flash->partitions[FLASH_SECTION_LOG_MONITOR].run = true;
		flash->partitions[FLASH_SECTION_LOG_MONITOR].init = true;

		snprintf(buf_msg, sizeof(buf_msg),
			"\r\n [SPI FLASH] MONITOR READY\r\n"
			" |- Hw : Sec %d B | Pag %d B\r\n"
			" |- Sec: %lu [0x%04lX] -> Dir: 0x%06lX\r\n"
			" |- Pag: %lu [0x%02lX] -> Dir: 0x%06lX | Off: %lu\r\n"
			" |- Cnt: %lu\r\n"
			" \\- Sys: Run=%d | Init=%d\r\n\r\n",
			FLASH_SECTOR_SIZE_4K,
			FLASH_PAGE_SIZE,

			(unsigned long)flash->partitions[FLASH_SECTION_LOG_MONITOR].current_sector,
			(unsigned long)flash->partitions[FLASH_SECTION_LOG_MONITOR].current_sector,
			(unsigned long)(flash->partitions[FLASH_SECTION_LOG_MONITOR].current_sector * FLASH_SECTOR_SIZE_4K),

			(unsigned long)flash->partitions[FLASH_SECTION_LOG_MONITOR].current_page,
			(unsigned long)flash->partitions[FLASH_SECTION_LOG_MONITOR].current_page,
			(unsigned long)((flash->partitions[FLASH_SECTION_LOG_MONITOR].current_sector * FLASH_SECTOR_SIZE_4K) +
							(flash->partitions[FLASH_SECTION_LOG_MONITOR].current_page * FLASH_PAGE_SIZE)),

			(unsigned long)flash->partitions[FLASH_SECTION_LOG_MONITOR].sector_offset,
			(unsigned long long)flash->partitions[FLASH_SECTION_LOG_MONITOR].sectionMonitor.monitor.contador,
			flash->partitions[FLASH_SECTION_LOG_MONITOR].run,
			flash->partitions[FLASH_SECTION_LOG_MONITOR].init
		);

		osDelay(TXFLASHLOG);
		MSG_Enviar(MSG_GENERAL,buf_msg, Uart1);
		osDelay(TXFLASHLOG);

		osDelay(TXFLASHLOG);
		MSG_Enviar(MSG_GENERAL,
			"\r\n==========================================================\r\n"
			" [SPI FLASH] [Flash_Monitor_Init] FINAL MONITOR_SPI\r\n"
			"==========================================================\r\n",
			Uart1
		);
		osDelay(TXFLASHLOG);
		return true;

	}



	// ENCONTRAMOS CENTINELA E INICALIZAMOS -->


	protectionLimit = Flas_Monitor_protetionLimit_Range(flash, start_addr, end_addr);

	if (!protectionLimit) {
		osDelay(TXFLASHLOG);
		MSG_Enviar(MSG_GENERAL,
			"\r\n [SPI FLASH FATAL] [Flash_Monitor_Init] Inicializacion abortada: fallo en limites de sectores con centinela encontrado.\r\n",
			Uart1
		);
		osDelay(TXFLASHLOG);
		osDelay(TXFLASHLOG);
		MSG_Enviar(MSG_GENERAL,
			"\r\n==========================================================\r\n"
			" [SPI FLASH] [Flash_Monitor_Init] FINAL MONITOR_SPI\r\n"
			"==========================================================\r\n",
			Uart1
		);
		osDelay(TXFLASHLOG);


		return false;
	}

	protectionLimit = Flas_Monitor_protetionLimit(flash, addrCentinela);

	if (!protectionLimit) {
		osDelay(TXFLASHLOG);
		MSG_Enviar(MSG_GENERAL,
			"\r\n [SPI FLASH FATAL] [Flash_Monitor_Init] Inicializacion abortada: fallo en limite de dirección del centinela.\r\n",
			Uart1
		);
		osDelay(TXFLASHLOG);
		osDelay(TXFLASHLOG);
		MSG_Enviar(MSG_GENERAL,
			"\r\n==========================================================\r\n"
			" [SPI FLASH] [Flash_Monitor_Init] FINAL MONITOR_SPI\r\n"
			"==========================================================\r\n",
			Uart1
		);
		osDelay(TXFLASHLOG);


		return false;
	}



	flash->partitions[FLASH_SECTION_LOG_MONITOR].current_sector = addrCentinela / FLASH_SECTOR_SIZE_4K;
	flash->partitions[FLASH_SECTION_LOG_MONITOR].current_page = 0;
	flash->partitions[FLASH_SECTION_LOG_MONITOR].sector_offset = 0;
	flash->partitions[FLASH_SECTION_LOG_MONITOR].sectionMonitor.monitor.contador = (contador + 1);

	flash->partitions[FLASH_SECTION_LOG_MONITOR].run = true;
	flash->partitions[FLASH_SECTION_LOG_MONITOR].init = true;

	snprintf(buf_msg, sizeof(buf_msg),
		"\r\n [SPI FLASH] MONITOR READY\r\n"
		" |- Hw : Sec %d B | Pag %d B\r\n"
		" |- Sec: %lu [0x%04lX] -> Dir: 0x%06lX\r\n"
		" |- Pag: %lu [0x%02lX] -> Dir: 0x%06lX | Off: %lu\r\n"
		" |- Cnt: %lu\r\n"
		" \\- Sys: Run=%d | Init=%d\r\n\r\n",
		FLASH_SECTOR_SIZE_4K,
		FLASH_PAGE_SIZE,

		(unsigned long)flash->partitions[FLASH_SECTION_LOG_MONITOR].current_sector,
		(unsigned long)flash->partitions[FLASH_SECTION_LOG_MONITOR].current_sector,
		(unsigned long)(flash->partitions[FLASH_SECTION_LOG_MONITOR].current_sector * FLASH_SECTOR_SIZE_4K),

		(unsigned long)flash->partitions[FLASH_SECTION_LOG_MONITOR].current_page,
		(unsigned long)flash->partitions[FLASH_SECTION_LOG_MONITOR].current_page,
		(unsigned long)((flash->partitions[FLASH_SECTION_LOG_MONITOR].current_sector * FLASH_SECTOR_SIZE_4K) +
						(flash->partitions[FLASH_SECTION_LOG_MONITOR].current_page * FLASH_PAGE_SIZE)),

		(unsigned long)flash->partitions[FLASH_SECTION_LOG_MONITOR].sector_offset,
		(unsigned long long)flash->partitions[FLASH_SECTION_LOG_MONITOR].sectionMonitor.monitor.contador,
		flash->partitions[FLASH_SECTION_LOG_MONITOR].run,
		flash->partitions[FLASH_SECTION_LOG_MONITOR].init
	);

	osDelay(TXFLASHLOG);
	MSG_Enviar(MSG_GENERAL,buf_msg, Uart1);
	osDelay(TXFLASHLOG);

	Flash_Complement_Data_Thread(FLASH_DEBUG_THREAD_SIZE);

	osDelay(TXFLASHLOG);
	MSG_Enviar(MSG_GENERAL,
		"\r\n==========================================================\r\n"
		" [SPI FLASH] [Flash_Monitor_Init] FINAL MONITOR_SPI\r\n"
		"==========================================================\r\n",
		Uart1
	);
	osDelay(TXFLASHLOG);

	return true;
};
