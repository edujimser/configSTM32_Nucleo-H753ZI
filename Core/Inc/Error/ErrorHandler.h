/* errors.h */
#ifndef __ERRORS_H
#define __ERRORS_H

#include <stdint.h>

typedef enum {
    ERR_PRUEBA = 0x00,               // Código genérico para pruebas
    ERR_OSC_PLL = 0x01,              // Error de oscilador o PLL
    ERR_FLASH_LATENCY = 0x02,          // Error de latencia Flash
    ERR_RTC_INIT = 0x04,             // Error de inicialización del RTC  
    ERR_UART3_INIT = 0x05,           // Error de inicialización
    ERR_UART3_TX_FIFO = 0x06,        // Error FIFO Transmisión
    ERR_UART3_RX_FIFO = 0x07,        // Error FIFO Recepción
    ERR_UART3_FIFO_DISABLE = 0x08,    // Error al desactivar FIFO
    ERR_UART3_CLK_CONFIG = 0x09,     // Error al configurar el reloj de UART3
    ERR_HANDLER_HARD = 0X0A          // Error crítico en el HardFault_Handler
} CodeError_t; 
extern CodeError_t CodeError;

/**
 * @brief Estructura para el diagnóstico detallado de fallos del sistema.
 */
typedef struct {
    const char *file;        // Nombre del archivo (__FILE__)
    const char *function;    // Nombre de la función (__func__)
    uint32_t    line;        // Línea del error (__LINE__)
    uint32_t    err_code;    // Código numérico (ej: 0x01, 0x05)
    const char *err_name;    // Nombre corto (ej: "TIMEOUT_ERR")
    const char *description; // Descripción larga (ej: "El sensor I2C no respondió")
    uint32_t   timestamp;   // Marca de tiempo del error (opcional, requiere RTC)
} ErrorHandler_t; extern ErrorHandler_t ErrorHandler; 

/* Prototipo de la función de manejo de errores */
void Error_Handler(void);


/* Prototipo de la función de manejo de fallos hard */
void HardFault_Handler(void);

#endif
