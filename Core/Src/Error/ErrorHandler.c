/* errors.c */
#include "Error/ErrorHandler.h"
#include <stdio.h>
#include "stm32h7xx_hal.h"

// Traemos la instancia del RTC que está definida en main.c
extern RTC_HandleTypeDef hrtc;

/* Inicialización por defecto de la estructura */
ErrorHandler_t ErrorHandler = {
    .file = "Ninguno",
    .function = "Ninguna",
    .line = 0,
    .err_code = 0,
    .err_name = "OK",
    .description = "No hay errores detectados",
    .timestamp = 0,
};


/**
  * @brief  Implementación global del manejador de errores
*/

void Error_Handler(void)
{
  ErrorHandler.timestamp = HAL_GetTick(); // Marca de tiempo del error (en ms desde el arranque)

  RTC_TimeTypeDef sTime = {0};
  RTC_DateTypeDef sDate = {0};

  // Es fundamental leer la hora y luego la fecha para obtener datos coherentes
  if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK)
  {
    HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
  }

  /* 1. Diagnóstico por puerto serie */
  printf("\r\n###########################################################");
  printf("\r\n#                PÁNICO DEL SISTEMA (H7)                 #");
  printf("\r\n###########################################################");
  
  // A. Estampa de tiempo CRUDA (Ticks del sistema)
  printf("\r\n  TIMESTAMP (Raw)  : %lu ms desde el arranque", ErrorHandler.timestamp);
  
  // B. Estampa de tiempo HUMANA (Calendario RTC)
  // Usamos %02d para que siempre tenga 2 dígitos (ej: 09:05:01)
  printf("\r\n  FECHA Y HORA     : %02d/%02d/20%02d | %02d:%02d:%02d", 
          sDate.Date, sDate.Month, sDate.Year,
          sTime.Hours, sTime.Minutes, sTime.Seconds);
  
  printf("\r\n-----------------------------------------------------------");
  printf("\r\n  DETALLES DEL ERROR:");
  printf("\r\n  NOMBRE           : %s",   ErrorHandler.err_name);
  printf("\r\n  CÓDIGO           : 0x%08lX", ErrorHandler.err_code);
  printf("\r\n  DESCRIPCIÓN      : %s",   ErrorHandler.description);
  
  printf("\r\n-----------------------------------------------------------");
  printf("\r\n  UBICACIÓN DEL FALLO:");
  printf("\r\n  Archivo          : %s",   ErrorHandler.file);
  printf("\r\n  Función          : %s",   ErrorHandler.function);
  printf("\r\n  Línea            : %lu",  ErrorHandler.line);
  printf("\r\n###########################################################\r\n");

  /* 2. Estado físico de seguridad (LEDs) */
  // Apagamos indicadores normales (Verde/Amarillo)
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET); 
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_RESET); 
  HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_SET); 

  /* 3. Bloqueo de seguridad */
  __disable_irq(); // Desactiva todas las interrupciones para que nada más se mueva
  
  while (1)
  {
    // El sistema se queda aquí hasta que alguien pulse el botón de RESET
  }
}


void HardFault_Handler(void)
{
    __asm volatile
    (
        "tst lr, #4                        \n"
        "ite eq                            \n"
        "mrseq r0, msp                     \n"
        "mrsne r0, psp                     \n"
        "b HardFault_Handler_C             \n"
    );
}

void HardFault_Handler_C(uint32_t *stack)
{
    uint32_t r0  = stack[0];
    uint32_t r1  = stack[1];
    uint32_t r2  = stack[2];
    uint32_t r3  = stack[3];
    uint32_t r12 = stack[4];
    uint32_t lr  = stack[5];
    uint32_t pc  = stack[6];
    uint32_t psr = stack[7];

    uint32_t cfsr  = SCB->CFSR;
    uint32_t hfsr  = SCB->HFSR;
    uint32_t mmfar = SCB->MMFAR;
    uint32_t bfar  = SCB->BFAR;

    /* Rellenamos tu estructura ErrorHandler */
    ErrorHandler.file        = "HARDFAULT";
    ErrorHandler.function    = "HardFault_Handler_C";
    ErrorHandler.line        = pc;   // Guardamos PC como “línea”
    ErrorHandler.err_code    = ERR_HANDLER_HARD; // Código específico para HardFault
    ErrorHandler.err_name    = "HARDFAULT";
    ErrorHandler.description = "Fallo crítico del núcleo Cortex-M7";

    /* Imprimimos tu cabecera */
    ErrorHandler.timestamp = HAL_GetTick();

    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK)
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);

    printf("\r\n###########################################################");
    printf("\r\n#                PÁNICO DEL SISTEMA ERROR HARD (H7)       #");
    printf("\r\n###########################################################");

    printf("\r\n  TIMESTAMP (Raw)  : %lu ms", ErrorHandler.timestamp);
    printf("\r\n  FECHA Y HORA     : %02d/%02d/20%02d | %02d:%02d:%02d",
           sDate.Date, sDate.Month, sDate.Year,
           sTime.Hours, sTime.Minutes, sTime.Seconds);

    printf("\r\n-----------------------------------------------------------");
    printf("\r\n  DETALLES DEL ERROR:");
    printf("\r\n  NOMBRE           : %s",   ErrorHandler.err_name);
    printf("\r\n  CÓDIGO           : 0x%08lX", ErrorHandler.err_code);
    printf("\r\n  DESCRIPCIÓN      : %s",   ErrorHandler.description);

    printf("\r\n-----------------------------------------------------------");
    printf("\r\n  UBICACIÓN DEL FALLO:");
    printf("\r\n  PC (Program Ctr) : 0x%08lX", pc);
    printf("\r\n  LR (Link Reg)    : 0x%08lX", lr);
    printf("\r\n  PSR              : 0x%08lX", psr);

    printf("\r\n-----------------------------------------------------------");
    printf("\r\n  REGISTROS:");
    printf("\r\n  R0  = 0x%08lX", r0);
    printf("\r\n  R1  = 0x%08lX", r1);
    printf("\r\n  R2  = 0x%08lX", r2);
    printf("\r\n  R3  = 0x%08lX", r3);
    printf("\r\n  R12 = 0x%08lX", r12);

    printf("\r\n-----------------------------------------------------------");
    printf("\r\n  REGISTROS DE FALLO (SCB):");
    printf("\r\n  CFSR = 0x%08lX", cfsr);
    printf("\r\n  HFSR = 0x%08lX", hfsr);
    printf("\r\n  MMFAR= 0x%08lX", mmfar);
    printf("\r\n  BFAR = 0x%08lX", bfar);

    printf("\r\n###########################################################\r\n");

    /* LEDs de pánico */
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOE, GPIO_PIN_1, GPIO_PIN_RESET);
    HAL_GPIO_WritePin(GPIOB, GPIO_PIN_14, GPIO_PIN_SET);

    __disable_irq();
    while (1) {}
}
