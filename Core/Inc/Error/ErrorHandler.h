/**
 * @file ErrorHandler.h
 * @brief Sistema de gestión y diagnóstico de fallos críticos para STM32H7.
 * @author Tu Nombre / Departamento de Firmware
 * @date 2026
 * @version 1.1
 * * @details Este módulo proporciona las estructuras, enumeraciones y prototipos
 * necesarios para capturar, almacenar en "Caja Negra" y reportar fallos de 
 * software y excepciones de hardware (Faults) del núcleo ARM Cortex-M7.
 */

#ifndef __ERRORHANDLER_H
#define __ERRORHANDLER_H

/* ========================================================================= */
/* --- DEPENDENCIAS e INCLUSIONES                                        --- */
/* ========================================================================= */
#include <stdint.h>
#include <stdio.h>
#include "stm32h7xx_hal.h"
#include "LedDataVision/LedData.h"

/* ========================================================================= */
/* --- TIPOS DE DATOS Y ENUMERACIONES                                    --- */
/* ========================================================================= */

/**
 * @enum ErrorCode_t
 * @brief Códigos de error de software y excepciones de hardware del sistema.
 */
typedef enum {
    ERR_PRUEBA             = 0x00, /**< Código de test inicial o depuración libre. */
    ERR_OSC_PLL            = 0x01, /**< Fallo en la inicialización de osciladores o bucle PLL. */
    ERR_FLASH_LATENCY      = 0x02, /**< Error de configuración en los tiempos de espera (Latency) de la Flash. */
    ERR_RTC_INIT           = 0x04, /**< Fallo al inicializar o comunicar con el periférico RTC. */
    ERR_UART3_INIT         = 0x05, /**< Fallo en la configuración de registros de la línea UART3. */
    ERR_UART3_TX_FIFO      = 0x06, /**< Desbordamiento o error crítico en la cola FIFO de transmisión UART3. */
    ERR_UART3_RX_FIFO      = 0x07, /**< Desbordamiento o error crítico en la cola FIFO de recepción UART3. */
    ERR_UART3_FIFO_DISABLE = 0x08, /**< Fallo al intentar desactivar el modo FIFO en UART3. */
    ERR_UART3_CLK_CONFIG   = 0x09, /**< Error en la asignación del árbol de relojes hacia la UART3. */
    ERR_HANDLER_HARD       = 0X0A, /**< Excepción HardFault detectada (Fallo crítico general). */
    ERR_HANDLER_USAGEFAULT = 0X0B, /**< Excepción UsageFault detectada (Instrucción ilegal o división por 0). */
    ERR_HANDLER_BUSFAULT   = 0X0C, /**< Excepción BusFault detectada (Acceso de datos/instrucción síncrono/asíncrono). */
    ERR_HANDLER_MEMMANAGE  = 0X0D  /**< Excepción MemManage detectada (Violación de reglas de la MPU). */
} ErrorCode_t;

/**
 * @struct ErrorHandler_t
 * @brief Estructura de "Caja Negra" para el registro estático de fallos del sistema.
 */
typedef struct {
    const char *file;         /**< Puntero al nombre del archivo fuente donde ocurrió el fallo (__FILE__). */
    const char *function;     /**< Puntero al nombre de la función donde se invocó el error (__func__). */
    uint32_t    line;         /**< Número de línea exacta del archivo fuente (__LINE__) o dirección del PC. */
    uint32_t    err_code;     /**< Código numérico del error (Asociado a @ref ErrorCode_t). */
    const char *err_name;     /**< Cadena de texto corta identificativa del error. */
    const char *description;  /**< Descripción detallada del escenario de fallo o subregistro de CPU. */
    uint32_t    timestamp;    /**< Tiempo transcurrido en milisegundos desde el arranque del sistema (SysTick). */
} ErrorHandler_t; 

/* ========================================================================= */
/* --- VARIABLES GLOBALES EXTERNAS                                       --- */
/* ========================================================================= */

/**
 * @brief Instancia global de la caja negra de errores.
 */
extern ErrorHandler_t ErrorHandler; 

/* ========================================================================= */
/* --- PROTOTIPOS DE LAS FUNCIONES                                       --- */
/* ========================================================================= */

/**
 * @brief Inicializa y configura los registros de excepciones de hardware del núcleo.
 * @return void
 */
void Error_Handler_Init(void);

/**
 * @brief Manejador estándar para capturar fallos de Software en caliente.
 * @note Esta función utiliza el atributo @c naked para evitar la alteración del prólogo de pila.
 * @return void
 */
__attribute__((naked)) void Error_Handler(void);

/**
 * @brief Vector ISR interrupción nativa: HardFault_Handler.
 * @note Implementada en ensamblador (@c naked) para extraer el puntero de pila nativo.
 * @return void
 */
__attribute__((naked)) void HardFault_Handler(void);

/**
 * @brief Vector ISR interrupción nativa: UsageFault_Handler.
 * @note Implementada en ensamblador (@c naked) para extraer el puntero de pila nativo.
 * @return void
 */
__attribute__((naked)) void UsageFault_Handler(void);

/**
 * @brief Vector ISR interrupción nativa: BusFault_Handler.
 * @note Implementada en ensamblador (@c naked) para extraer el puntero de pila nativo.
 * @return void
 */
__attribute__((naked)) void BusFault_Handler(void);

/**
 * @brief Vector ISR interrupción nativa: MemManage_Handler.
 * @note Implementada en ensamblador (@c naked) para extraer el puntero de pila nativo.
 * @return void
 */
__attribute__((naked)) void MemManage_Handler(void);

#endif /* __ERRORHANDLER_H */