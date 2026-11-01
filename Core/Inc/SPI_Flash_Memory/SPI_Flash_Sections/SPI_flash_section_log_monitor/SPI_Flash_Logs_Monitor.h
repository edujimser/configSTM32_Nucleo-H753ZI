#ifndef SPI_FLASH_LOGS_MONITROR
#define SPI_FLASH_LOGS_MONITROR

#include "SPI_Flash_Memory/SPI_Flash_Memory.h"

#define HEADERBEBECAFEMORENUMBERPOSITION 12
#define NUMBER_SECTOR_MINIMUN 3


/* ==============================================================================
 * COMPLEMENTS
 * ============================================================================== */
bool Flas_Monitor_protetionLimit(SPI_Flash_TypeDef *flash, uint32_t addr);
bool Flas_Monitor_protetionLimit_Range(SPI_Flash_TypeDef *flash, uint32_t addrInit, uint32_t addrFinal);
/* ==============================================================================
 * TEST
 * ============================================================================== */
bool Test_Flash_Monitor_Inject_Valid_Data(SPI_Flash_TypeDef *flash);
bool Test_Flash_Monitor_Inject_Error_Data(SPI_Flash_TypeDef *flash);
bool Test_Flash_Monitor_Inject_Valid_Data_All(SPI_Flash_TypeDef *flash);
#endif /*SPI_FLASH_LOGS_MONITROR*/
