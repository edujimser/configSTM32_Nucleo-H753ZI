#ifndef EXTERNAL_FLASH_H
#define EXTERNAL_FLASH_H

/* ==============================================================================
 * INCLUDES Y DEPENDENCIAS
 * ============================================================================== */
#include "main.h"
#include <stdbool.h>
#include <stdint.h>
#include "cmsis_os2.h"
#include "SPI_Flash_Memory.h"

/* ==============================================================================
 * DIRECCIONES
 * ============================================================================== */
#define FLASH_SECTOR_SIZE  0x1000
#define GET_SECTOR_ADDR(sector) ((sector) * FLASH_SECTOR_SIZE)

/* ==============================================================================
 * 1. COMANDOS HARDWARE (JEDEC STANDARD)
 * ============================================================================== */
#define FLASH_CMD_BP            0X01  // Quita los bloqaueos de borrado
#define FLASH_CMD_WRITE_ENABLE  0x06  // Quita el seguro de escritura (WEL=1)
#define FLASH_CMD_READ_STATUS   0x05  // Lee el Registro de Estado 1
#define FLASH_CMD_READ          0x03  // Lee datos de forma estándar
#define FLASH_CMD_PAGE_PROGRAM  0x02  // Escribe datos (máximo 256 bytes)
#define FLASH_CMD_SECTOR_ERASE  0x20  // Borra 1 Sector (4 KB)
#define FLASH_CMD_CHIP_ERASE    0xC7  // Borra la memoria completa
#define FLASH_CMD_ID            0x9F  // Solicita fabricante y capacidad

/* ==============================================================================
 * 2. GEOMETRÍA DE LA MEMORIA (GigaDevice/Winbond 16MB)
 * ============================================================================== */
// Capacidades y Límites (Bytes)
#define FLASH_CAPACITY          16777216  // Capacidad total (16 MB)
#define FLASH_PAGE_SIZE         256       // Límite de escritura por ciclo
#define FLASH_SECTION_SIZE      4096      // Tamaño de un sector (4 KB)
#define FLASH_BLOCK_SIZE        65536     // Tamaño de un bloque (64 KB)


// Totales de divisiones físicas
#define FLASH_PAGE_COUNT        65536     // Total de páginas en el chip
#define FLASH_SECTOR_COUNT      4096      // Total de sectores en el chip
#define FLASH_BLOCK_COUNT       256       // Total de bloques en el chip
#define FLASH_NUM_PAGE          (FLASH_SECTION_SIZE/FLASH_PAGE_SIZE)
/* ==============================================================================
 * 3. TEMPORIZADORES Y TIMEOUTS (Milisegundos)
 * ============================================================================== */
#define WRITE_READ_NORMAL_VELOCITY 100    // Timeout para SPI HAL general
#define WAIT_FOR_READY             300    // Timeout corto de espera (estado)
#define WAIT_FOR_READY_FAIL        1000   // Intervalo para logs de ocupado
#define ERASE_ONE_SECTOR           1000   // Tiempo máx para borrar 1 Sector
#define ERASE_ONE_BLOQ             3000   // Tiempo máx para borrar 1 Bloque
#define ERASE_ALL_FLASH            100000 // Tiempo máx para borrar el Chip
#define STATUS_FOR_READY	       500    // Tiempo de consulta de status de chips
#define TXFLASHLOG                 100    // Tiempo de liberación del hilo

/* ==============================================================================
 * 4. MAPA DE MEMORIA DINÁMICO Y ESTRUCTURAS
 * ============================================================================== */
typedef enum {
    PART_CONFIG = 0,
    PART_LOGS,
    PART_DATA,
    PARTITION_COUNT
} Flash_Partition_ID;

typedef struct {
	uint16_t        id;              // Id
    uint16_t 		start_sector;    // Sector inicial de la partición
    uint16_t 		end_sector;      // Sector final de la partición
    uint16_t 		sector_count;    // Cantidad total de sectores asignados

    uint16_t 		current_sector;  // Puntero lógico: Sector actual de trabajo
    uint16_t 		sector_offset;   // Puntero lógico: Bytes escritos en el sector actual
    bool     		is_configured;   // Bandera de seguridad de inicialización

} Flash_Partition_TypeDef;

typedef struct {
    SPI_HandleTypeDef       *hspi;      // Puntero al periférico SPI del STM32
    GPIO_TypeDef            *cs_port;   // Puerto GPIO del Chip Select (CS)
    uint16_t                cs_pin;     // Pin GPIO del Chip Select (CS)
    bool                    ocuped;     // Bandera Mutex para el RTOS
    osMutexId_t		        spi_mutex;   // Mutex SPI

    Flash_Partition_TypeDef partitions[PARTITION_COUNT]; // Mapa de particiones

} SPI_Flash_TypeDef;

/* ==============================================================================
 * 5. VARIABLES GLOBALES EXPORTADAS
 * ============================================================================== */
extern SPI_Flash_TypeDef spi_flash_memory_1;

/* ==============================================================================
 * 6. PROTOTIPOS DE FUNCIONES (API DE USUARIO)
 * ============================================================================== */

// --- Inicialización y Configuración ---
void Flash_Init(SPI_Flash_TypeDef *flash, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin, bool mensaje);
void Flash_Read_ID_Init(SPI_Flash_TypeDef *flash);
bool Flash_Config_Partition_Init(SPI_Flash_TypeDef *flash, Flash_Partition_ID part_id, uint16_t start_sector, uint16_t num_sectors);
bool Flash_Check_Partition_Overlap(SPI_Flash_TypeDef *flash, Flash_Partition_ID part_id, uint16_t start_sector, uint16_t end_sector);
bool Flash_detect_chip_Init(SPI_Flash_TypeDef *flash);

// --- Estatus ---
void Flash_Read_ID(SPI_Flash_TypeDef *flash);
void Flash_Read_Component(SPI_Flash_TypeDef *flash);
bool Flash_Is_Sector_partition(SPI_Flash_TypeDef *flash, Flash_Partition_ID partition, uint32_t dir);
bool Flash_HaveData_Sector(SPI_Flash_TypeDef *flash, Flash_Partition_ID partition, uint32_t dir);

// --- Habilitaciones --
bool Flash_Wait_For_Ready(SPI_Flash_TypeDef *flash, uint32_t timeout_ms);
bool Flash_Write_Enable(SPI_Flash_TypeDef *flash);

// --- Borrado ---
bool Flash_Erase_All(SPI_Flash_TypeDef *flash);
bool Flash_Erase_All_For_Sector(SPI_Flash_TypeDef *flash, uint32_t sectors);
bool Flash_Erase_One_Sector(SPI_Flash_TypeDef *flash, Flash_Partition_ID partition, uint32_t dirs);

// --- Escritura ---
bool Flash_Write_All_Sectors_Data(SPI_Flash_TypeDef *flash, uint32_t total_sectors, uint32_t pages_per_sector, uint8_t data_value);
bool Flash_Write_One_Sector(SPI_Flash_TypeDef *flash, Flash_Partition_ID partition, uint32_t sector, const uint8_t *data, uint32_t dataLenght);

// --- Lectura Dump ---
bool Flash_Dump_All_Sectors(SPI_Flash_TypeDef *flash);
bool Flash_Dump_One_Sectors_Hex(SPI_Flash_TypeDef *flash, uint32_t sector);
bool Flash_Dump_One_Sectors_ASCII(SPI_Flash_TypeDef *flash, uint32_t sector);
bool Flash_Dump_One_Page_Hex(SPI_Flash_TypeDef *flash, uint32_t sector,  uint8_t page);
bool Flash_Dump_One_Page_ASCII(SPI_Flash_TypeDef *flash, uint32_t sector, uint8_t page);


#endif /* EXTERNAL_FLASH_H */
