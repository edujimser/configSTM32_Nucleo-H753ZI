#include "LedDataVision/LedData.h"





const osThreadAttr_t StartGreenYellowRedTask_attributes = {
  .name = "TaskError",                            // Nombre para identificar la tarea en herramientas de depuración.
  .stack_size = 256 * 4,                          // Reserva 1024 bytes (256 palabras de 32 bits) de RAM para la pila de esta tarea.
  .priority = (osPriority_t) osPriorityLow1 ,     // Asignamos una prioridad baja para que esta tarea no interfiera con tareas críticas.
};


LedConfig_t startConfiguration = {
    .mode = MODE_SEQUENCE,
    .delayMs = BLINK_VERY_SLOW,
    .toggleState = false,
    .lastToggleTime = 0,
    .toggleCounter = 0,
    .modePanic = false,
    .modePanicTimeOut = BLINK_PANIC_OFF,
};


void applyLedConfiguration(LedConfig_t *config) {
    switch (config->mode) {
        
        // ==========================================
        // MODOS FIJOS
        // ==========================================
        case MODE_ALL_ON:
            LED_GREEN_ON();
            LED_YELLOW_ON();
            LED_RED_ON();
            break;
        case MODE_ONLY_RED:
            LED_RED_ON();
            LED_GREEN_OFF();
            LED_YELLOW_OFF();
            break;
        case MODE_ONLY_YELLOW:
            LED_YELLOW_ON();
            LED_GREEN_OFF();
            LED_RED_OFF();
            break;
        case MODE_ONLY_GREEN:
            LED_GREEN_ON();
            LED_YELLOW_OFF();
            LED_RED_OFF();
            break;
        case MODE_ONLY_RED_YELLOW:
            LED_RED_ON();
            LED_YELLOW_ON();
            LED_GREEN_OFF();
            break;
        case MODE_ONLY_RED_GREEN:
            LED_RED_ON();
            LED_GREEN_ON();
            LED_YELLOW_OFF();
            break;
        case MODE_ONLY_YELLOW_GREEN:
            LED_YELLOW_ON();
            LED_GREEN_ON();
            LED_RED_OFF();
            break;

        // ==========================================
        // MODOS PARPADEO
        // ==========================================
        case MODE_SEQUENCE:
            // Si está activada la bandera modePanic, entra directo. Si no, evalúa HAL_GetTick()
            if (config->modePanic || ((HAL_GetTick() - config->lastToggleTime) >= config->delayMs)) {
                switch (config->toggleCounter) {
                    case 0:
                        LED_GREEN_ON();
                        LED_YELLOW_OFF();
                        LED_RED_OFF();
                        break;
                    case 1:
                        LED_GREEN_OFF();
                        LED_YELLOW_ON();
                        LED_RED_OFF();
                        break;
                    case 2:
                        LED_GREEN_OFF();
                        LED_YELLOW_OFF();
                        LED_RED_ON();
                        break;
                }
                config->toggleCounter = (config->toggleCounter + 1) % 3;
                config->lastToggleTime = HAL_GetTick();

                // Si estamos atrapados en pánico, hacemos un retraso manual para que el ojo humano lo vea
                if (config->modePanic) {
                    for (volatile uint32_t i = 0; i < 6000000; i++);
                }
            }
            break;

        case MODE_BLINK_ALL:
            if (config->modePanic || ((HAL_GetTick() - config->lastToggleTime) >= config->delayMs)) {
                LED_GREEN_WRITE(config->toggleState);
                LED_YELLOW_WRITE(config->toggleState);
                LED_RED_WRITE(config->toggleState);

                config->lastToggleTime = HAL_GetTick();
                config->toggleState = !config->toggleState;

                if (config->modePanic) {
                    for (volatile uint32_t i = 0; i < 6000000; i++);
                }
            }
            break;

        case MODE_RED_BLINK:
            if (config->modePanic || ((HAL_GetTick() - config->lastToggleTime) >= config->delayMs)) {
                LED_RED_WRITE(config->toggleState);
                LED_GREEN_OFF();
                LED_YELLOW_OFF();

                config->lastToggleTime = HAL_GetTick();
                config->toggleState = !config->toggleState;

                if (config->modePanic) {
                    for (volatile uint32_t i = 0; i < 6000000; i++);
                }
            }
            break;

        case MODE_YELLOW_BLINK:
            if (config->modePanic || ((HAL_GetTick() - config->lastToggleTime) >= config->delayMs)) {
                LED_YELLOW_WRITE(config->toggleState);
                LED_GREEN_OFF();
                LED_RED_OFF();

                config->lastToggleTime = HAL_GetTick();
                config->toggleState = !config->toggleState;

                if (config->modePanic) {
                    for (volatile uint32_t i = 0; i < 6000000; i++);
                }
            }
            break;

        case MODE_GREEN_BLINK:
            if (config->modePanic || ((HAL_GetTick() - config->lastToggleTime) >= config->delayMs)) {
                LED_GREEN_WRITE(config->toggleState);
                LED_YELLOW_OFF();
                LED_RED_OFF();

                config->lastToggleTime = HAL_GetTick();
                config->toggleState = !config->toggleState;

                if (config->modePanic) {
                    for (volatile uint32_t i = 0; i < 6000000; i++);
                }
            }
            break;

        case MODE_RED_YELLOW_BLINK:
            if (config->modePanic || ((HAL_GetTick() - config->lastToggleTime) >= config->delayMs)) {
                LED_RED_WRITE(config->toggleState);
                LED_YELLOW_WRITE(config->toggleState);
                LED_GREEN_OFF();

                config->lastToggleTime = HAL_GetTick();
                config->toggleState = !config->toggleState;

                if (config->modePanic) {
                    for (volatile uint32_t i = 0; i < 6000000; i++);
                }
            } 
            break;

        case MODE_RED_GREEN_BLINK:
            if (config->modePanic || ((HAL_GetTick() - config->lastToggleTime) >= config->delayMs)) {
                LED_RED_WRITE(config->toggleState);
                LED_GREEN_WRITE(config->toggleState);
                LED_YELLOW_OFF();

                config->lastToggleTime = HAL_GetTick();
                config->toggleState = !config->toggleState;

                if (config->modePanic) {
                    for (volatile uint32_t i = 0; i < 6000000; i++);
                }
            }
            break;

        case MODE_YELLOW_GREEN_BLINK:
            if (config->modePanic || ((HAL_GetTick() - config->lastToggleTime) >= config->delayMs)) {
                LED_YELLOW_WRITE(config->toggleState);
                LED_GREEN_WRITE(config->toggleState);
                LED_RED_OFF();

                config->lastToggleTime = HAL_GetTick();
                config->toggleState = !config->toggleState;

                if (config->modePanic) {
                    for (volatile uint32_t i = 0; i < 6000000; i++);
                }
            }
            break;

        case MODE_OFF:
        default:
            LED_GREEN_OFF();
            LED_YELLOW_OFF();
            LED_RED_OFF();
            config->lastToggleTime = 0;
            config->toggleState = 0;
            break;
    }   
}



/**
  * @brief Tarea que gestiona LEDs basándose en una estructura de configuración.
  * @param argument: Puntero genérico (void*) que recibimos del main al crear la tarea.
  */
void StartGreenYellowRedTask(void *argument) {
    
    /* --- 1. PREPARACIÓN DE DATOS --- */

    // Convertimos el puntero 'void*' a un puntero tipo 'LedConfig_t*'.
    // Esto es necesario porque 'void*' no tiene "forma", y así el compilador sabe cómo leer la estructura.
    LedConfig_t *config = (LedConfig_t *)argument;

    // Validar que el modo esté dentro del rango permitido de la lista (Enum)
    if (config->mode >= END_OF_MODES  || config->mode < 0) {
        config->mode = MODE_SEQUENCE; 
    }

    // Validar que el delay no sea 0 (Blink_off) para evitar fallos de tiempo
    if (config->delayMs == BLINK_OFF || config->delayMs >= END_OF_BLINKS) {
        config->delayMs = BLINK_VERY_SLOW; //
    }
    
    /* --- 2. BUCLE INFINITO DEL HILO --- */
    for(;;) {
        applyLedConfiguration(config);
        osDelay(config->delayMs); 
    }
}
