/* LedData.h */
#ifndef __LED_DATA_H
#define __LED_DATA_H

// 1. Librerías del sistema
#include <stdio.h>
#include <stdint.h>    
#include <stdbool.h> 

// 2. Dependencias de hardware y sistema operativo
#include "stm32h7xx_hal.h"    
#include "cmsis_os2.h"

// ==========================================
// CONFIGURACIÓN DE HARDWARE (Mapeo de Pines)
// ==========================================
#define LED_GREEN_PORT       GPIOB
#define LED_GREEN_PIN        GPIO_PIN_0

#define LED_YELLOW_PORT      GPIOE
#define LED_YELLOW_PIN       GPIO_PIN_1

#define LED_RED_PORT         GPIOB
#define LED_RED_PIN          GPIO_PIN_14

// ==========================================
// Macros de control rápido para encender, apagar y escribir estados
// ==========================================
#define LED_GREEN_ON()       HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_SET)
#define LED_GREEN_OFF()      HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, GPIO_PIN_RESET)
#define LED_GREEN_WRITE(s)   HAL_GPIO_WritePin(LED_GREEN_PORT, LED_GREEN_PIN, (s) ? GPIO_PIN_SET : GPIO_PIN_RESET)

#define LED_YELLOW_ON()      HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, GPIO_PIN_SET)
#define LED_YELLOW_OFF()     HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, GPIO_PIN_RESET)
#define LED_YELLOW_WRITE(s)  HAL_GPIO_WritePin(LED_YELLOW_PORT, LED_YELLOW_PIN, (s) ? GPIO_PIN_SET : GPIO_PIN_RESET)

#define LED_RED_ON()         HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_SET)
#define LED_RED_OFF()        HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, GPIO_PIN_RESET)
#define LED_RED_WRITE(s)     HAL_GPIO_WritePin(LED_RED_PORT, LED_RED_PIN, (s) ? GPIO_PIN_SET : GPIO_PIN_RESET)

// ==========================================
// Delays de parpadeo
// ==========================================
#define BLINK_OFF        0      // Estado estático / apagado
#define BLINK_FAST       100    // Parpadeo de 0.1 segundos
#define BLINK_NO_SATURE  250    // Parpadeo de 0.25 segundos
#define BLINK_SLOW       500    // Parpadeo de 0.5 segundos
#define BLINK_VERY_SLOW  1000   // Parpadeo de 1.0 segundo
#define BLINK_4_SECONDS  4000   // Parpadeo de 4.0 segundos
#define BLINK_8_SECONDS  8000   // Parpadeo de 8.0 segundos
#define BLINK_12_SECONDS 12000  // Parpadeo de 12.0 segundos
#define END_OF_BLINKS    15000  // Nuevo límite máximo para validaciones (15 segundos)

// ==========================================
// Delays de parpadeo - MODO PÁNICO (Software Loops)
// Calibrados para coincidir con la tabla superior a ~400MHz
// ==========================================
#define BLINK_PANIC_OFF           0UL
#define BLINK_PANIC_FAST          2400000UL   // Equivale a ~100 ms
#define BLINK_PANIC_NO_SATURE     6000000UL   // Equivale a ~250 ms
#define BLINK_PANIC_SLOW          12000000UL  // Equivale a ~500 ms
#define BLINK_PANIC_VERY_SLOW     24000000UL  // Equivale a ~1000 ms (1s)
#define BLINK_PANIC_4_SECONDS     96000000UL  // Equivale a ~4.0 segundos
#define BLINK_PANIC_8_SECONDS     192000000UL // Equivale a ~8.0 segundos
#define BLINK_PANIC_12_SECONDS    288000000UL // Equivale a ~12.0 segundos

// ==========================================
// Listado de modos de operación para los LEDs
// ==========================================
typedef enum {
    MODE_ALL_ON = 0,         
    MODE_SEQUENCE,           
    MODE_BLINK_ALL,          
    MODE_ONLY_RED,           
    MODE_ONLY_YELLOW,        
    MODE_ONLY_GREEN,         
    MODE_ONLY_RED_YELLOW,    
    MODE_ONLY_RED_GREEN,     
    MODE_ONLY_YELLOW_GREEN,  
    MODE_RED_BLINK,          
    MODE_YELLOW_BLINK,       
    MODE_GREEN_BLINK,
    MODE_RED_YELLOW_BLINK,
    MODE_RED_GREEN_BLINK,
    MODE_YELLOW_GREEN_BLINK,        
    MODE_OFF,                
    END_OF_MODES             
} LedMode_t;


// ==========================================
// Estructura de configuración para los LEDs
// ==========================================
typedef struct {
    LedMode_t mode;          
    uint32_t delayMs;        
    bool toggleState;        
    uint32_t lastToggleTime; 
    uint32_t toggleCounter;  
    bool modePanic;
    uint32_t modePanicTimeOut;
} LedConfig_t;


// ==========================================
// Declaración de funciones públicas y variables externas
// ==========================================
void StartGreenYellowRedTask(void *argument);
void applyLedConfiguration(LedConfig_t *config);

extern const osThreadAttr_t StartGreenYellowRedTask_attributes;
extern LedConfig_t startConfiguration;
extern LedConfig_t configErrorLed;

#endif /* __LED_DATA_H */