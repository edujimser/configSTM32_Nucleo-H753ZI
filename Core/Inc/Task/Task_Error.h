/* USER CODE BEGIN Header */
/**
  * @file           : app_tasks.h
  * @brief          : Definiciones y prototipos para las tareas del sistema
  */
/* USER CODE END Header */

#ifndef __APP_TASKS_ERROR_H__
#define __APP_TASKS_ERROR_H__

#include <stdint.h>    /* Para tipos de datos enteros de tamaño fijo */
#include "cmsis_os2.h" /* API del SO para reconocer tipos como osThreadId_t */
#include "main.h"      /* Para reconocer los HAL_GPIO y pines */

typedef enum {
    MODE_ALL_ON = 0,         // Todos los LEDs encendidos
    MODE_SEQUENCE,           // Los tres LEDs uno detrás de otro
    MoDE_BLINK_ALL,          // Todos los LEDs parpadeando al mismo tiempo

    MODE_ONLY_RED,           // Solo el rojo
    MODE_ONLY_YELLOW,        // Solo el amarillo
    MODE_ONLY_GREEN,         // Solo el verde

    MODE_RED_BLINK,          // Solo el rojo parpadeando
    MODE_YELLOW_BLINK,       // Solo el amarillo parpadeando
    MODE_GREEN_BLINK,        // Solo el verde parpadeando

    MODE_ONLY_RED_YELLOW,    // Solo rojo y amarillo encendidos
    MODE_ONLY_RED_GREEN,     // Solo rojo y verde encendidos
    MODE_ONLY_YELLOW_GREEN,  // Solo amarillo y verde encendidos  

    END_OF_MODES             // Marca el final de los modos disponibles

} LedMode_t;

typedef enum {
    BLINK_FAST      = 500,
    BLINK_NORMAL    = 1000,
    BLINK_SLOW      = 2000,
    BLINK_VERY_SLOW = 5000
} BlinkSpeed_t;


typedef struct {
    LedMode_t mode;           // Qué modo queremos
    BlinkSpeed_t delayMs; // Cuánto tiempo queremos que dure cada estado (en ms)
} LedConfig_t;



/* ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ---                                                                          PROTOTIPOS DE TAREAS                                                                         --- */
/* ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- */

/**
  * @brief Funciones que ejecutan el bucle infinito de cada hilo
  */

void StartGreenYellowRedTask(void *argument);

/* ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ---                                                                          ATRIBUTOS EXTERNOS                                                                           --- */
/* ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- */

/**
  * @brief Declaramos los atributos como 'extern' para que el main.c pueda verlos
  */

extern const osThreadAttr_t StartGreenYellowRedTask_attributes;
extern LedConfig_t startConfiguration;
extern LedConfig_t errorConfiguration;

#endif /* __APP_TASKS_ERROR_H__ */