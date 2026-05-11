#include "Task/Task_Error.h"





const osThreadAttr_t StartGreenYellowRedTask_attributes = {
  .name = "TaskError",                            // Nombre para identificar la tarea en herramientas de depuración.
  .stack_size = 256 * 4,                          // Reserva 1024 bytes (256 palabras de 32 bits) de RAM para la pila de esta tarea.
  .priority = (osPriority_t) osPriorityRealtime,  // Establece una prioridad estándar (ni muy alta ni muy baja).
};


LedConfig_t startConfiguration = {
    .mode = MODE_SEQUENCE,
    .delayMs = BLINK_FAST
};

/**
  * @brief Tarea que gestiona LEDs basándose en una estructura de configuración.
  * @param argument: Puntero genérico (void*) que recibimos del main al crear la tarea.
  */
void StartGreenYellowRedTask(void *argument) {
    
    /* --- 1. PREPARACIÓN DE DATOS --- */

    // Convertimos el puntero 'void*' a un puntero tipo 'LedConfig_t*'.
    // Esto es necesario porque 'void*' no tiene "forma", y así el compilador sabe cómo leer la estructura.
    LedConfig_t *config = (LedConfig_t *)argument;

    // Operador ternario: Si 'config' no es NULL, usamos su delay. Si es NULL, ponemos 500ms por seguridad
    // para evitar que la tarea se ejecute a velocidad infinita y bloquee el CPU.
    uint32_t dly = (config != NULL) ? config->delayMs : 500;
    
    /* --- 2. BUCLE INFINITO DEL HILO --- */
for(;;) {
        // Si por error config es NULL, usamos valores seguros
        LedMode_t modoActual = (config != NULL) ? config->mode : MODE_ONLY_RED;
        uint32_t dly = (config != NULL) ? (uint32_t)config->delayMs : 500;

        switch (modoActual) {

            case MODE_ALL_ON:
                /* Todos encendidos fijos */
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);  // Verde
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_SET);  // Amarillo
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET); // Rojo
                osDelay(dly); // Delay de cortesía para no saturar el CPU
                break;

            case MODE_SEQUENCE:
                /* Secuencia: Verde -> Amarillo -> Rojo */
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
                osDelay(dly);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_SET);
                osDelay(dly);
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
                osDelay(dly);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
                break;

            case MoDE_BLINK_ALL:
                /* Todos parpadean al mismo tiempo */
                HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
                HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_1);
                HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
                osDelay(dly);
                break;

            case MODE_ONLY_RED:
                /* Solo rojo encendido fijo */
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_RESET);
                osDelay(dly);
                break;

            case MODE_ONLY_YELLOW:
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
                osDelay(dly);
                break;

            case MODE_ONLY_GREEN:
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
                osDelay(dly);
                break;

            case MODE_RED_BLINK:
                HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_14);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_RESET);
                osDelay(dly);
                break;

            case MODE_YELLOW_BLINK:
                HAL_GPIO_TogglePin(GPIOE, GPIO_PIN_1);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
                osDelay(dly);
                break;

            case MODE_GREEN_BLINK:
                HAL_GPIO_TogglePin(GPIOB, GPIO_PIN_0);
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
                osDelay(dly);
                break;

            case MODE_ONLY_RED_YELLOW:
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
                osDelay(dly);
                break;

            case MODE_ONLY_RED_GREEN:
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_RESET);
                osDelay(dly);
                break;

            case MODE_ONLY_YELLOW_GREEN:
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_SET);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
                osDelay(dly);
                break;

            default:
                /* Si algo falla, apaga todo por seguridad */
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_RESET);
                HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET);
                osDelay(dly);
                break;
        }
    }
}
