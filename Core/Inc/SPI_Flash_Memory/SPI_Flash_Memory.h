#ifndef EXTERNAL_FLASH_H
#define EXTERNAL_FLASH_H

/* ==============================================================================
 * 1. INCLUDES Y DEPENDENCIAS
 * ============================================================================== */
#include "main.h"
#include <stdbool.h>
#include <stdint.h>
#include "cmsis_os2.h"


/* ==============================================================================
 * COMANDOS HARDWARE (JEDEC STANDARD)
 * ============================================================================== */
#define FLASH_CMD_BP            0X01  // Quita los bloqueos de borrado
#define FLASH_CMD_WRITE_ENABLE  0x06  // Quita el seguro de escritura (WEL=1)
#define FLASH_CMD_READ_STATUS   0x05  // Lee el Registro de Estado 1
#define FLASH_CMD_READ          0x03  // Lee datos de forma estándar
#define FLASH_CMD_READ_RAPID    0x0B  // Lee datos de forma rapida
#define FLASH_CMD_PAGE_PROGRAM  0x02  // Escribe datos (máximo 256 bytes)
#define FLASH_CMD_SECTOR_ERASE  0x20  // Borra 1 Sector (4 KB)
#define FLASH_CMD_CHIP_ERASE    0xC7  // Borra la memoria completa
#define FLASH_CMD_ID            0x9F  // Solicita fabricante y capacidad
#define FLASH_WAKE_UP           0xAB  // Solicita despertarse, salida del modo bajo consumo

//RESET SW
#define CMD_RESET_ENABLE        0X66 // 1º Comando de inicio de reset
#define CMD_RESET_EXECUTE       0x99 // 2º Comando reset

/* ==============================================================================
 * GEOMETRÍA DE LA MEMORIA (GigaDevice/Winbond 16MB)
 * ============================================================================== */
// Capacidades y Límites Físicos (Bytes)
#define FLASH_PAGE_SIZE         256       // Límite de escritura por ciclo (1 Página)
#define FLASH_SECTION_SIZE      4096      // Tamaño de un sector (4 KB)
#define FLASH_BLOCK_SIZE        65536     // Tamaño de un bloque (64 KB)
#define FLASH_SECTOR_SIZE_4K    0x1000    // Tamaño de sector en Hexadecimal (4 KB)

// Totales de divisiones físicas
#define FLASH_PAGE_COUNT        65536     // Total de páginas en el chip
#define FLASH_num_sector_count  4096      // Total de sectores en el chip
#define FLASH_BLOCK_COUNT       256       // Total de bloques en el chip


// Macros de cálculo de direcciones
#define GET_SECTOR_ADDR(sector) ((sector) * FLASH_SECTOR_SIZE_4K)
#define GET_FLASH_CAPCITY       (FLASH_BLOCK_COUNT * FLASH_BLOCK_SIZE)
#define GET_FLASH_NUM_PAGE      (FLASH_SECTION_SIZE / FLASH_PAGE_SIZE)


/* ==============================================================================
 * TEMPORIZADORES Y TIMEOUTS (Milisegundos)
 * ============================================================================== */
#define WRITE_READ_NORMAL_VELOCITY 100    // Timeout para SPI HAL general
#define WAIT_FOR_READY             300    // Timeout corto de espera (estado)
#define WAIT_FOR_READY_FAIL        1000   // Intervalo para logs de ocupado
#define ERASE_ONE_SECTOR           1000   // Tiempo máx para borrar 1 Sector
#define ERASE_ONE_BLOQ             3000   // Tiempo máx para borrar 1 Bloque
#define ERASE_ALL_FLASH            100000 // Tiempo máx para borrar el Chip entero
#define STATUS_FOR_READY           500    // Tiempo de consulta de status de chips
#define TXFLASHLOG                 100    // Tiempo de liberación del hilo
#define ERASE_ALL_WAIT_BUSY        100000 // Tiempo de respues de flash despues de borrado masivo

/* ==============================================================================
 * FIRMAS
 * ============================================================================== */

#define FLASH_MAGIC_BEBECAFE    ((uint32_t)0xBEBECAFEUL)
#define FLASH_MAGIC_CENTINELA   ((uint32_t)0xFFFFFFFFUL)
#define FLASH_SECRET_NUMER      ((uint32_t)0x12345678UL)

/* ==============================================================================
 * DEBUG
 * ============================================================================== */
#define FLASH_DEBUG_FLASH              1
#define FLASH_DEBUG_THREAD_SIZE        0
#define FLASH_DEBUG_THREAD_SIZE_LIMIT  200

/* ==============================================================================
 * CONTROL SYSTEM
 * ============================================================================== */

#define FLASH_RESET_INIT 3
#define FLASH_RESET_EXEC 50

/* ==============================================================================
 * CONTROL SYSTEM
 * ============================================================================== */
#define STRUCT_NAME_PARTITION 50
/* ==============================================================================
 * PARTICIONES
 * ============================================================================== */
typedef enum {
    FLASH_SECTION_LOAD = 0,
    FLASH_SECTION_LOG_MONITOR,
    FLASH_SECTION_OTHER,
    FLASH_SECTION_COUNT_END
} Flash_Partition_ID;


/* ==============================================================================
 * ESTRUCTURA FLASH SPECIFIC PARTITION
 * ============================================================================== */
typedef struct {
    uint64_t contador;        // Número de secuencia global del log

} Flash_Monitor_Ext_TypeDef;

/* ==============================================================================
 * ESTRUCTURA FLASH
 * ============================================================================== */
// --- Control de Partición Individual ---
typedef struct {
    uint16_t id;              // ID de la partición (Flash_Partition_ID)
    bool     configured;      // Bandera: true si la partición fue montada correctamente
    bool     init;            // Bandera: sistema iniciasodIniciado
    bool     run;             // Corriendo

    char    name[STRUCT_NAME_PARTITION];

    uint32_t num_start_sector;// Numerico de sector inicial
	uint32_t num_end_sector;  // Numerico de sector final
	uint32_t num_sector_count;    // Cantidad total de sectores de la partición

    uint32_t start_sector;    // Sector inicial asignado
    uint32_t end_sector;      // Sector final asignado


    uint32_t current_sector;  // Puntero lógico: Sector actual donde se está escribiendo
    uint32_t current_page;    // Puntero lógico: Pagina del sector actual
    uint32_t sector_offset;   // Puntero lógico: Bytes ya escritos en el sector actual

    union {
    	Flash_Monitor_Ext_TypeDef monitor;
    } sectionMonitor;

} Flash_Partition_TypeDef;

// --- Instancia General de la Memoria Flash ---
typedef struct {
    SPI_HandleTypeDef       *hspi;      // Puntero al periférico SPI del STM32 (HAL)
    GPIO_TypeDef            *cs_port;   // Puerto GPIO del Chip Select (CS)
    uint16_t                cs_pin;     // Pin GPIO del Chip Select (CS)
    osMutexId_t             spi_mutex;  // Mutex para el RTOS (FreeRTOS)

    uint8_t                 reset_init; // Limite de reinicios en el arranque
    uint8_t                 reset_exec; // Limite de reinicios en ejecucion
    bool 					init;       // Bandera de estado inicio
    bool 					run;        // Bandera de estado run
    bool                    load;       // Bandera de carga de datos
    bool                    ocuped;     // Bandera de Ocupacion


    Flash_Partition_TypeDef partitions[FLASH_SECTION_COUNT_END]; // Array con el estado de las particiones
} SPI_Flash_TypeDef;




/* ==============================================================================
 * MASTERT SECTOR
 * ============================================================================== */
typedef struct {
    uint32_t format_key;
    Flash_Partition_TypeDef partitions[FLASH_SECTION_COUNT_END];
    uint16_t checksum;
} Master_Boot_Record_TypeDef;



/* ==============================================================================
 * VARIABLES GLOBALES EXPORTADAS
 * ============================================================================== */
extern SPI_Flash_TypeDef spi_flash_memory_1;


/* ==============================================================================
 * INIT
 * ============================================================================== */
void Flash_Init(SPI_Flash_TypeDef *flash, SPI_HandleTypeDef *hspi, GPIO_TypeDef *cs_port, uint16_t cs_pin, bool mensaje);
bool Flash_Init_Reset_SPI(SPI_Flash_TypeDef *flash, uint32_t codePositionReset);
bool Flash_Init_detect_chip(SPI_Flash_TypeDef *flash);
bool Flash_Init_Config_Partition(SPI_Flash_TypeDef *flash, Flash_Partition_ID part_id, uint32_t start_sector, uint32_t num_sectors, const char *namePartition);
bool Flash_Init_Check_Partition_Overlap(SPI_Flash_TypeDef *flash, Flash_Partition_ID part_id, uint32_t start_sector, uint32_t end_sector);

/* ==============================================================================
 * LOAD
 * ============================================================================== */
bool Flash_Load_Init(SPI_Flash_TypeDef *flash);
bool Flash_Load_Read(SPI_Flash_TypeDef *flash, uint32_t absolute_address, uint8_t *buffer, uint32_t length);
bool Flash_Load_Check(SPI_Flash_TypeDef *flash);
bool Flash_Load_Check_MBR_Checksum(Master_Boot_Record_TypeDef *mbr);
uint16_t Flash_Calculate_Check_MBR_Checksum(Master_Boot_Record_TypeDef *mbr);
bool Flash_Load_Erase_All_Sectors(SPI_Flash_TypeDef *flash);
bool Flash_Load_Write_Sector(SPI_Flash_TypeDef *flash, uint32_t sector, const uint8_t *data, uint32_t dataLenght);

//EN ESEPRA
bool Flash_Monitor_Init(SPI_Flash_TypeDef *flash);

/* ==============================================================================
 * ESTATUS
 * ============================================================================== */
void Flash_Read_ID(SPI_Flash_TypeDef *flash);
void Flash_Read_Component(SPI_Flash_TypeDef *flash);
/* ==============================================================================
 * READ
 * ============================================================================== */
bool Flash_read_One_Sector_Search(SPI_Flash_TypeDef *flash, uint32_t addr, uint32_t sizeSearchData, uint8_t *data);
bool Flash_read_One_Sector_Search_No_Mutex(SPI_Flash_TypeDef *flash, uint32_t addr, uint32_t sizeSearchData, uint8_t *data);
/* ==============================================================================
 * READ
 * ============================================================================== */
bool Flash_Write_All_Sectors_Data(SPI_Flash_TypeDef *flash, uint32_t total_sectors, uint32_t pages_per_sector, uint8_t data_value);
bool Flash_Write_One_Sector_Partition(SPI_Flash_TypeDef *flash, Flash_Partition_ID partition, uint32_t sector, uint32_t dataLenght , const uint8_t *data);

/* ==============================================================================
 * ERASE
 * ============================================================================== */
bool Flash_Erase_All(SPI_Flash_TypeDef *flash);
bool Flash_Erase_One_Hex_For_Sector(SPI_Flash_TypeDef *flash, uint32_t addr);
bool Flash_Erase_One_Sector_For_Sector(SPI_Flash_TypeDef *flash, uint32_t sector);
bool Flash_Erase_Range_Hex_For_Sector(SPI_Flash_TypeDef *flash, uint32_t addrInit, uint32_t addrFinal);
bool Flash_Erase_Range_Sector_For_Sector(SPI_Flash_TypeDef *flash, uint32_t sectorInit, uint32_t SectorFinal);
bool Flash_Erase_Partition(SPI_Flash_TypeDef *flash, Flash_Partition_ID partition);
/* ==============================================================================
 * COMANDOS UART
 * ============================================================================== */
typedef enum{
	VISION_HEXADECIMAL,
	VISION_ASCII,
	VISION_HEXADECIMAL_AND_ASCII,
	VISION_TYPE_END,
}E_CMD_UART_Read;

bool Flash_CMD_UART_Read_All_Sectors(SPI_Flash_TypeDef *flash, E_CMD_UART_Read option);
bool Flash_CMD_UART_Read_Range_Sectors_Page(SPI_Flash_TypeDef *flash, E_CMD_UART_Read option, uint32_t sectorInit, uint32_t sectorFinal, uint16_t numPage);
bool Flash_CMD_UART_Read_EnumSector(SPI_Flash_TypeDef *flash, E_CMD_UART_Read option, uint32_t ID);
bool Flash_CMD_UART_Read_EnumSector_Sectors(SPI_Flash_TypeDef *flash, E_CMD_UART_Read option, uint32_t ID, uint32_t sectorInit, uint32_t sectorFinal);



/* ==============================================================================
 * COMPLEMENTS
 * ============================================================================== */
void Flash_Complement_Data_Thread(bool debugSystem);
bool Flash_Complement_Is_Sector_partition(SPI_Flash_TypeDef *flash, Flash_Partition_ID partition, uint32_t dir);
bool Flash_Comeplement_Wait_For_Ready(SPI_Flash_TypeDef *flash, uint32_t timeout_ms);
bool Flash_Complement_HaveData_Sector(SPI_Flash_TypeDef *flash, Flash_Partition_ID partition, uint32_t dir);
bool Flash_Complement_Write_Enable(SPI_Flash_TypeDef *flash);
bool Flash_Complement_Ocuped(SPI_Flash_TypeDef *flash, char forcedFree);
bool Flash_Wait_Busy(SPI_Flash_TypeDef *flash, uint32_t timeout_ms, uint32_t *timeUse);
bool Flash_Complement_protetionLimit(SPI_Flash_TypeDef *flash, uint32_t addr);
bool Flash_Complement_protetionLimit_Range(SPI_Flash_TypeDef *flash, uint32_t addrInit, uint32_t addrFinal);

#endif /* EXTERNAL_FLASH_H */
