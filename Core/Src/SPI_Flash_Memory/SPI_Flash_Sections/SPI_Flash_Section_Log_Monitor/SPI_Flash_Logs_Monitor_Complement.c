

#include <string.h>
#include <stdio.h>

#include "cmsis_os.h"


#include "SPI_Flash_Memory/SPI_Flash_Sections/SPI_flash_section_log_monitor/SPI_Flash_Logs_Monitor.h"
#include "MsgUART/TxUart/TxUart.h"

extern I_TestUart TestUart_1;

bool Flas_Monitor_protetionLimit(SPI_Flash_TypeDef *flash, uint32_t addr) {
	uint32_t start_addr = flash->partitions[FLASH_SECTION_LOG_MONITOR].start_sector;
	uint32_t end_addr   = flash->partitions[FLASH_SECTION_LOG_MONITOR].end_sector;

	if(addr < start_addr || addr > end_addr) {

        char buf_err[150];
        snprintf(buf_err, sizeof(buf_err), "\r\n [PROT LIMIT FAIL] Addr pedida: %lu | Start RAM: %lu | End RAM: %lu\r\n",
                 (unsigned long)addr, (unsigned long)start_addr, (unsigned long)end_addr);

        osDelay(TXFLASHLOG);
        MSG_Enviar(buf_err, TestUart_1);
        osDelay(TXFLASHLOG);

		return false;
	}
	return true;
}

bool Flas_Monitor_protetionLimit_Range(SPI_Flash_TypeDef *flash, uint32_t addrInit, uint32_t addrFinal) {
	uint32_t start_addr = flash->partitions[FLASH_SECTION_LOG_MONITOR].start_sector ;
	uint32_t end_addr   = flash->partitions[FLASH_SECTION_LOG_MONITOR].end_sector;

	if(addrInit < start_addr || addrFinal > end_addr) {
		return false;
	}
	return true;
}

bool Test_Flash_Monitor_Inject_Valid_Data(SPI_Flash_TypeDef *flash) {

	flash->partitions[FLASH_SECTION_LOG_MONITOR].run = false;

	osDelay(TXFLASHLOG);
	MSG_Enviar(
		"\r\n==========================================================\r\n"
		" [SPI FLASH] [Flash_Monitor_Test] MONITOR_SPI\r\n"
		"==========================================================\r\n",
		TestUart_1
	);
	osDelay(TXFLASHLOG);


	if (!flash->init || !flash->load ) {
		osDelay(TXFLASHLOG);
		MSG_Enviar("\r\n [SPI FLASH FATAL] Inicializacion abortada: SPI o Load no iniciado.\r\n", TestUart_1);
		osDelay(TXFLASHLOG);
		MSG_Enviar(
			"\r\n==========================================================\r\n"
			" [SPI FLASH] [Flash_Monitor_Init] FINAL MONITOR_SPI\r\n"
			"==========================================================\r\n",
			TestUart_1
		);
		osDelay(TXFLASHLOG);

		return false;
	}

	char buf_msg[250];
	uint32_t start_addr = flash->partitions[FLASH_SECTION_LOG_MONITOR].start_sector;
	uint32_t end_addr   = flash->partitions[FLASH_SECTION_LOG_MONITOR].end_sector;
	bool protectionLimit = false;

	protectionLimit = Flas_Monitor_protetionLimit_Range(flash, start_addr, end_addr);

	if (!protectionLimit) {
		osDelay(TXFLASHLOG);
		MSG_Enviar("\r\n [SPI FLASH FATAL] Inicializacion abortada: fallo en limites de sectores.\r\n", TestUart_1);
		osDelay(TXFLASHLOG);

		return false;
	}

	osDelay(TXFLASHLOG);
	MSG_Enviar("\r\n Inyectando y verificando 3 Sectores Validos...\r\n", TestUart_1);
	osDelay(TXFLASHLOG);

	if (!Flash_Erase_Range_Hex_For_Sector(flash, start_addr, end_addr)) {
		MSG_Enviar("[TEST ERROR] Fallo al borrar particion.\r\n", TestUart_1);
		return false;
	}

	// Búfer estricto de 12 bytes (4 bytes de Magic + 8 bytes de Secuencia)
	uint8_t bufferCode[12];
	uint8_t rx_buffercode[12];

	// =========================================================================
	// ESCRITURA
	// =========================================================================
	for (int i = 0; i < 3; i++) {
		uint32_t sector_addr = start_addr + (i * FLASH_SECTION_SIZE);
		uint32_t magic = FLASH_MAGIC_BEBECAFE;
		uint32_t centinela = (uint64_t)((i+1) * 7);

		memcpy(&bufferCode[0], &magic, sizeof(magic));
		memcpy(&bufferCode[4], &centinela, sizeof(centinela));

		protectionLimit = Flas_Monitor_protetionLimit(flash, sector_addr);

		if (!protectionLimit) {
			osDelay(TXFLASHLOG);
			MSG_Enviar("[TEST] Bucle escritura con fallo de direccion, saltando...\r\n", TestUart_1);
			osDelay(TXFLASHLOG);
			continue;
		}

		Flash_Write_One_Sector_Partition(flash, FLASH_SECTION_LOG_MONITOR, sector_addr, sizeof(bufferCode), bufferCode);
	}

	osDelay(TXFLASHLOG);
	MSG_Enviar("[TEST OK] Escritura finalizada. Comprobando memoria...\r\n", TestUart_1);
	osDelay(TXFLASHLOG);

	// =========================================================================
	// LECTURA Y VERIFICACIÓN
	// =========================================================================
	uint32_t read_magic;
	uint32_t read_secret;

	for (int i = 0; i < 4; i++) {
		uint32_t sector_addr = start_addr + (i * FLASH_SECTION_SIZE);


		Flash_read_One_Sector_Search(flash, sector_addr, sizeof(rx_buffercode), rx_buffercode);

		memcpy(&read_magic, &rx_buffercode[0], sizeof(read_magic));
		memcpy(&read_secret, &rx_buffercode[4], sizeof(read_secret));

		// 3. Imprimir usando %llu para el uint64_t
		snprintf(buf_msg, sizeof(buf_msg), " |- Sec %d [0x%06lX]: Mag=0x%08lX | SecNum=%llu\r\n",
				 i, (unsigned long)sector_addr, (unsigned long)read_magic, (unsigned long long)read_secret);

		osDelay(TXFLASHLOG);
		MSG_Enviar(buf_msg, TestUart_1);
		osDelay(TXFLASHLOG);
	}

	// El retorno y los mensajes finales deben ir FUERA del bucle
	osDelay(TXFLASHLOG);
	MSG_Enviar(" \\- Sec 3 [Centinela]: Esperando ser detectado (Magic=0xFFFFFFFF)\r\n\r\n", TestUart_1);
	osDelay(TXFLASHLOG);

	osDelay(500);

	__disable_irq();
	NVIC_SystemReset();

	return true;
}

bool Test_Flash_Monitor_Inject_Valid_Data_All(SPI_Flash_TypeDef *flash) {

	flash->partitions[FLASH_SECTION_LOG_MONITOR].run = false;

	osDelay(TXFLASHLOG);
	MSG_Enviar(
		"\r\n==========================================================\r\n"
		" [SPI FLASH] [Flash_Monitor_Test] MONITOR_SPI\r\n"
		"==========================================================\r\n",
		TestUart_1
	);
	osDelay(TXFLASHLOG);


	if (!flash->init || !flash->load ) {
		osDelay(TXFLASHLOG);
		MSG_Enviar("\r\n [SPI FLASH FATAL] Inicializacion abortada: SPI o Load no iniciado.\r\n", TestUart_1);
		osDelay(TXFLASHLOG);
		MSG_Enviar(
			"\r\n==========================================================\r\n"
			" [SPI FLASH] [Flash_Monitor_Init] FINAL MONITOR_SPI_ALL\r\n"
			"==========================================================\r\n",
			TestUart_1
		);
		osDelay(TXFLASHLOG);

		return false;
	}

	char buf_msg[250];
	uint32_t start_addr = flash->partitions[FLASH_SECTION_LOG_MONITOR].start_sector;
	uint32_t end_addr   = flash->partitions[FLASH_SECTION_LOG_MONITOR].end_sector;
	uint32_t num_sector_count = flash->partitions[FLASH_SECTION_LOG_MONITOR].num_sector_count;
	bool protectionLimit = false;

	protectionLimit = Flas_Monitor_protetionLimit_Range(flash, start_addr, end_addr);

	if (!protectionLimit) {
		osDelay(TXFLASHLOG);
		MSG_Enviar("\r\n [SPI FLASH FATAL] Inicializacion abortada: fallo en limites de sectores.\r\n", TestUart_1);
		osDelay(TXFLASHLOG);

		return false;
	}

	osDelay(TXFLASHLOG);
	MSG_Enviar("\r\n Inyectando y verificando datos...\r\n", TestUart_1);
	osDelay(TXFLASHLOG);



	// =========================================================================
	// ESCRITURA
	// =========================================================================
	// Búfer estricto de 12 bytes (4 bytes de Magic + 8 bytes de Secuencia)
	uint8_t bufferCode[12];
	uint8_t rx_buffercode[12];


	for (int i = 0; i < num_sector_count; i++) {
		uint32_t sector_addr = start_addr + (i * FLASH_SECTION_SIZE);
		uint32_t magic = FLASH_MAGIC_BEBECAFE;
		uint32_t centinela = (uint64_t)(i+1);

		memcpy(&bufferCode[0], &magic, sizeof(magic));
		memcpy(&bufferCode[4], &centinela, sizeof(centinela));

		protectionLimit = Flas_Monitor_protetionLimit(flash, sector_addr);

		if (!protectionLimit) {
			osDelay(TXFLASHLOG);
			MSG_Enviar("[TEST] Bucle escritura con fallo de direccion, saltando...\r\n", TestUart_1);
			osDelay(TXFLASHLOG);
			continue;
		}

		Flash_Write_One_Sector_Partition(flash, FLASH_SECTION_LOG_MONITOR, sector_addr, sizeof(bufferCode), bufferCode);
	}

	osDelay(TXFLASHLOG);
	MSG_Enviar("[TEST OK] Escritura finalizada. Comprobando memoria...\r\n", TestUart_1);
	osDelay(TXFLASHLOG);

	// =========================================================================
	// LECTURA Y VERIFICACIÓN
	// =========================================================================
	uint32_t read_magic;
	uint64_t read_secret;

	for (int i = 0; i < num_sector_count; i++) {
		uint32_t sector_addr = start_addr + (i * FLASH_SECTION_SIZE);


		Flash_read_One_Sector_Search(flash, sector_addr, sizeof(rx_buffercode), rx_buffercode);

		memcpy(&read_magic, &rx_buffercode[0], sizeof(read_magic));
		memcpy(&read_secret, &rx_buffercode[4], sizeof(read_secret));

		// 3. Imprimir usando %llu para el uint64_t
		snprintf(buf_msg, sizeof(buf_msg), " |- Sec %d [0x%06lX]: Mag=0x%08lX | SecNum=%lu\r\n",
				 i, (unsigned long)sector_addr, (unsigned long)read_magic, (unsigned long)read_secret);

		osDelay(TXFLASHLOG);
		MSG_Enviar(buf_msg, TestUart_1);
		osDelay(TXFLASHLOG);
	}

	osDelay(TXFLASHLOG);
	MSG_Enviar("\r\n [SPI FLASH FATAL] Sistema inyectado reiniciando...\r\n", TestUart_1);
	osDelay(TXFLASHLOG);
	MSG_Enviar(
		"\r\n==========================================================\r\n"
		" [SPI FLASH] [Flash_Monitor_Init] FINAL MONITOR_SPI_ALL\r\n"
		"==========================================================\r\n",
		TestUart_1
	);
	osDelay(TXFLASHLOG);

	osDelay(500);
	//meter mensaje aqui
	__disable_irq();
	NVIC_SystemReset();

	return true;
}



