/**
 * @file LedData.h
 * @brief Módulo de control visual, gestión de estados y abstracción de LEDs.
 * @author Tu Nombre / Departamento de Firmware
 * @date 2026
 * @version 1.2
 * @details Este archivo contiene las definiciones de hardware, macros de control 
 * rápido, enumeraciones de modos de señalización y las estructuras necesarias 
 * para el manejo de LEDs tanto en hilos del RTOS (CMSIS-OS2) como en bucles de pánico.
 */

#ifndef __LED_DATA_H
#define __LED_DATA_H

/* ========================================================================= */
/* --- DEPENDENCIAS E INCLUSIONES                                        --- */
/* ========================================================================= */
#include <stdio.h>
#include <stdint.h>    
#include <stdbool.h> 
#include "stm32h7xx_hal.h"    
#include "cmsis_os2.h"

/* ========================================================================= */
/* --- CONFIGURACIÓN DE HARDWARE (MAPEO DE PINES)                        --- */
/* ========================================================================= */
#define LED_GREEN_PORT       GPIOB          /**< Puerto GPIO asignado al LED Verde. */
#define LED_GREEN_PIN        GPIO_PIN_0     /**< Pin GPIO asignado al LED Verde. */

#define LED_YELLOW_PORT      GPIOE          /**< Puerto GPIO asignado al LED Amarillo. */
#define LED_YELLOW_PIN       GPIO_PIN_1     /**< Pin GPIO asignado al LED Amarillo. */

#define LED_RED_PORT         GPIOB          /**< Puerto GPIO asignado al LED Rojo. */
#define LED_RED_PIN          GPIO_PIN_14    /**< Pin GPIO asignado al LED Rojo. */

/* ========================================================================= */
/* --- MACROS DE ACCESO RÁPIDO Y CONTROL DE HARDWARE                     --- */
/* ========================================================================= */

/** @brief Fuerza el encendido inmediato del LED Verde. */
#define LED_GREEN_ON()       HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_SET)
/** @brief Fuerza el apagado inmediato del LED Verde. */
#define LED_GREEN_OFF()      HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_RESET)
/** @brief Escribe un estado dinámico en el LED Verde basado en una condición lógica. */
#define LED_GREEN_WRITE(s)   HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, (s) ? GPIO_PIN_SET : GPIO_PIN_RESET)

/** @brief Fuerza el encendido inmediato del LED Amarillo. */
#define LED_YELLOW_ON()      HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, GPIO_PIN_SET)
/** @brief Fuerza el apagado inmediato del LED Amarillo. */
#define LED_YELLOW_OFF()     HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, GPIO_PIN_RESET)
/** @brief Escribe un estado dinámico en el LED Amarillo basado en una condición lógica. */
#define LED_YELLOW_WRITE(s)  HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, (s) ? GPIO_PIN_SET : GPIO_PIN_RESET)

/** @brief Fuerza el encendido inmediato del LED Rojo. */
#define LED_RED_ON()         HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_SET)
/** @brief Fuerza el apagado inmediato del LED Rojo. */
#define LED_RED_OFF()        HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET)
/** @brief Escribe un estado dinámico en el LED Rojo basado en una condición lógica. */
#define LED_RED_WRITE(s)     HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, (s) ? GPIO_PIN_SET : GPIO_PIN_RESET)

/* ========================================================================= */
/* --- CONSTANTES DE TIEMPO (DELAYS PARA PARPADEO EN RTOS)               --- */
/* ========================================================================= */
#define BLINK_OFF        0      /**< Estado estático / Sin parpadeo activo. */
#define BLINK_FAST       100    /**< Parpadeo rápido: Intervalo de 100 ms. */
#define BLINK_NO_SATURE  250    /**< Parpadeo intermedio cómodo: Intervalo de 250 ms. */
#define BLINK_SLOW       500    /**< Parpadeo síncrono estándar: Intervalo de 500 ms. */
#define BLINK_VERY_SLOW  1000   /**< Parpadeo pausado: Intervalo de 1.0 segundo. */
#define BLINK_4_SECONDS  4000   /**< Intervalo largo de ráfaga: 4.0 segundos. */
#define BLINK_8_SECONDS  8000   /**< Intervalo largo de ráfaga: 8.0 segundos. */
#define BLINK_12_SECONDS 12000  /**< Intervalo máximo de ciclo: 12.0 segundos. */
#define END_OF_BLINKS    15000  /**< Límite superior de seguridad para validación de tiempos. */

/* ========================================================================= */
/* --- CONSTANTES DE TIEMPO - MODO PÁNICO (SOFTWARE POLLING DELAYS)      --- */
/* ========================================================================= */
#define BLINK_PANIC_OFF           0UL           /**< Lazo de retraso desactivado en modo de fallo. */
#define BLINK_PANIC_FAST          2400000UL     /**< Lazo equivalente a ~100 ms ejecutados a 400MHz. */
#define BLINK_PANIC_NO_SATURE     6000000UL     /**< Lazo equivalente a ~250 ms ejecutados a 400MHz. */
#define BLINK_PANIC_SLOW          12000000UL    /**< Lazo equivalente a ~500 ms ejecutados a 400MHz. */
#define BLINK_PANIC_VERY_SLOW     24000000UL    /**< Lazo equivalente a ~1000 ms (1s) ejecutados a 400MHz. */
#define BLINK_PANIC_4_SECONDS     96000000UL    /**< Lazo equivalente a ~4.0 segundos ejecutados a 400MHz. */
#define BLINK_PANIC_8_SECONDS     192000000UL   /**< Lazo equivalente a ~8.0 segundos ejecutados a 400MHz. */
#define BLINK_PANIC_12_SECONDS    288000000UL   /**< Lazo equivalente a ~12.0 segundos ejecutados a 400MHz. */

/* ========================================================================= */
/* --- TIPOS DE DATOS Y ENUMERACIONES                                    --- */
/* ========================================================================= */

/**
 * @enum LedMode_t
 * @brief Modos de operación y combinaciones visuales permitidas para la matriz de LEDs.
 */
typedef enum {
    MODE_ALL_ON = 0,         /**< Todos los indicadores encendidos simultáneamente. */
    MODE_SEQUENCE,           /**< Modo secuencial cíclico (Verde -> Amarillo -> Rojo). */
    MODE_BLINK_ALL,          /**< Parpadeo síncrono general de todos los LEDs a la vez. */
    MODE_ONLY_RED,           /**< Canal Rojo encendido fijo; el resto apagados. */
    MODE_ONLY_YELLOW,        /**< Canal Amarillo encendido fijo; el resto apagados. */
    MODE_ONLY_GREEN,         /**< Canal Verde encendido fijo; el resto apagados. */
    MODE_ONLY_RED_YELLOW,    /**< Canales Rojo y Amarillo fijos; Verde apagado. */
    MODE_ONLY_RED_GREEN,     /**< Canales Rojo y Verde fijos; Amarillo apagado. */
    MODE_ONLY_YELLOW_GREEN,  /**< Canales Amarillo y Verde fijos; Rojo apagado. */
    MODE_RED_BLINK,          /**< Parpadeo exclusivo del canal Rojo; resto apagados. */
    MODE_YELLOW_BLINK,       /**< Parpadeo exclusivo del canal Amarillo; resto apagados. */
    MODE_GREEN_BLINK,        /**< Parpadeo exclusivo del canal Verde; resto apagados. */
    MODE_RED_YELLOW_BLINK,   /**< Parpadeo síncrono de Rojo y Amarillo; Verde apagado. */
    MODE_RED_GREEN_BLINK,    /**< Parpadeo síncrono de Rojo y Verde; Amarillo apagado. */
    MODE_YELLOW_GREEN_BLINK, /**< Parpadeo síncrono de Amarillo y Verde; Rojo apagado. */
    MODE_OFF,                /**< Apagado general preventivo de todos los canales. */
    END_OF_MODES             /**< Límite superior del enum para control de límites de memoria. */
} LedMode_t;

/**
 * @struct LedConfig_t
 * @brief Contexto de configuración y variables de control dinámico de los LEDs.
 */
typedef struct {
    LedMode_t mode;             /**< Modo visual activo seleccionado de @ref LedMode_t. */
    uint32_t  delayMs;          /**< Tiempo de refresco de parpadeo en ms (Fase del RTOS). */
    bool      toggleState;      /**< Flag de estado lógico actual (Intermitencia true/false). */
    uint32_t  lastToggleTime;   /**< Registro del valor de HAL_GetTick() en la última conmutación. */
    uint32_t  toggleCounter;    /**< Contador interno de pasos de estado para modos secuenciales. */
    bool      modePanic;        /**< Flag prioritario: true si el sistema colapsó y corre sin RTOS. */
    uint32_t  modePanicTimeOut; /**< Valor del límite de ciclos asignado para los bucles for de pánico. */
} LedConfig_t;

/* ========================================================================= */
/* --- PROTOTIPOS DE FUNCIONES PÚBLICAS Y ENLACE EXTERNO                 --- */
/* ========================================================================= */

/**
 * @brief Hilo de ejecución (Task) del RTOS encargado del refresco cíclico de los LEDs.
 * @param[in] argument Puntero genérico que debe ser casteado a @ref LedConfig_t.
 */
void StartGreenYellowRedTask(void *argument);

/**
 * @brief Máquina de estados básica que aplica las acciones físicas sobre los GPIOs.
 * @param[in] config Puntero a la estructura de control de configuración @ref LedConfig_t.
 */
void applyLedConfiguration(LedConfig_t *config);

;
/** @brief Instancia de configuración inicial por defecto del firmware. */
extern LedConfig_t startConfiguration;
/** @brief Instancia global compartida reservada para el manejador de fallos catastróficos. */
extern LedConfig_t configErrorLed;

#endif /* __LED_DATA_H */
