/* errors.c */
#include "Error/ErrorHandler.h"
#include "LedDataVision/LedData.h"
#include "main.h"

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

LedConfig_t configErrorLed = {
    .mode = MODE_OFF,
    .delayMs = BLINK_NO_SATURE,
    .modePanic = false,
    .modePanicTimeOut = BLINK_PANIC_OFF,
};


/* ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ---                                                                          FUNCIONES                                                                                    --- */
/* ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- */
void Error_Handler_Init(void)
{
    /* Habilitar UsageFault, BusFault y MemManageFault en el System Control Block (SCB) */
    SCB->SHCSR |= SCB_SHCSR_USGFAULTENA_Msk |  // Fallos de uso (ej: división por cero)
                  SCB_SHCSR_BUSFAULTENA_Msk |  // Fallos de Bus (ej: relojes apagados)
                  SCB_SHCSR_MEMFAULTENA_Msk;   // Fallos de memoria/MPU

    
     SCB->CCR |= SCB_CCR_DIV_0_TRP_Msk; 
}


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
  // Apagamos indicadores normales (Verde/Amarillo) dejamos rojo fijo para indicar fallo crítico
   configErrorLed.mode = MODE_ONLY_RED;
   configErrorLed.delayMs = BLINK_OFF;
   configErrorLed.modePanic = false; 
   configErrorLed.modePanicTimeOut = BLINK_PANIC_OFF;

   applyLedConfiguration(&configErrorLed);

  /* 3. Bloqueo de seguridad */
  __disable_irq(); // Desactiva todas las interrupciones para que nada más se mueva
  
  while (1)
  {
    applyLedConfiguration(&configErrorLed);
  }
}


__attribute__((naked)) void HardFault_Handler(void)
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
    configErrorLed.mode = MODE_ONLY_YELLOW;
    configErrorLed.delayMs = BLINK_OFF;
    configErrorLed.modePanic = true; 
    configErrorLed.modePanicTimeOut = BLINK_PANIC_OFF;
    applyLedConfiguration(&configErrorLed);

    __disable_irq();

    while (1) {
      applyLedConfiguration(&configErrorLed);
    }
}


/**
 * @brief  Manejador de excepción de UsageFault en ensamblador.
 * @note   Detecta qué puntero de pila (MSP o PSP) estaba activo cuando colapsó 
 *         la CPU y se lo pasa como argumento a la función en C.
 */
__attribute__((naked)) void UsageFault_Handler(void)
{
    __asm volatile (
        "tst lr, #4 \n"          /* Comprueba el bit 2 de EXEC_RETURN (LR) */
        "ite eq \n"              /* Si es 0, el sistema usaba MSP. Si es 1, PSP */
        "mrseq r0, msp \n"       /* R0 = Main Stack Pointer */
        "mrsne r0, psp \n"       /* R0 = Process Stack Pointer */
        "b UsageFault_Handler_C \n" /* Salta a la función en C pasando R0 como parámetro */
    );
}

/**
 * @brief  Procesador del UsageFault en C.
 * @param  stack Puntero al bloque de la pila salvado automáticamente por la CPU.
 */
void UsageFault_Handler_C(uint32_t *stack)
{
    /* 1. Volcado de los registros básicos guardados en la pila */
    uint32_t r0  = stack[0];
    uint32_t r1  = stack[1];
    uint32_t r2  = stack[2];
    uint32_t r3  = stack[3];
    uint32_t r12 = stack[4];
    uint32_t lr  = stack[5];
    uint32_t pc  = stack[6];
    uint32_t psr = stack[7];

    /* 2. Captura de los registros de diagnóstico del SCB */
    uint32_t cfsr  = SCB->CFSR;
    uint32_t hfsr  = SCB->HFSR;
    uint32_t mmfar = SCB->MMFAR;
    uint32_t bfar  = SCB->BFAR;

    /* 3. Rellenamos tu estructura global ErrorHandler */
    ErrorHandler.file     = "USAGEFAULT";
    ErrorHandler.function = "UsageFault_Handler_C";
    ErrorHandler.line     = pc; 
    ErrorHandler.err_code = ERR_HANDLER_USAGEFAULT; 
    ErrorHandler.err_name = "USAGE_FAULT";

    /* 4. Análisis dinámico del registro CFSR para escribir la descripción */
    if (cfsr & SCB_CFSR_DIVBYZERO_Msk) 
    {
        ErrorHandler.description = "Fallo de Uso: Intento de ejecucion de una division por cero (DIVBYZERO)";
    } 
    else if (cfsr & SCB_CFSR_UNDEFINSTR_Msk) 
    {
        ErrorHandler.description = "Fallo de Uso: La CPU intento ejecutar una instruccion indefinida o corrupta (UNDEFINSTR)";
    } 
    else if (cfsr & SCB_CFSR_INVSTATE_Msk) 
    {
        ErrorHandler.description = "Fallo de Uso: Intento ilegal de ejecutar codigo en estado no valido (INVSTATE)";
    } 
    else if (cfsr & SCB_CFSR_UNALIGNED_Msk) 
    {
        ErrorHandler.description = "Fallo de Uso: Acceso a memoria no alineado detectado (UNALIGNED)";
    } 
    else 
    {
        ErrorHandler.description = "Fallo de Uso: Error de ejecucion crítico en el nucleo ARM Cortex-M7";
    }

    /* 5. Estampa de tiempo y lectura del calendario RTC */
    ErrorHandler.timestamp = HAL_GetTick();

    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK)
    {
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    }

    /* 6. Diagnóstico por puerto serie con tu formato exacto de banners */
    printf("\r\n###########################################################");
    printf("\r\n#                PÁNICO DEL SISTEMA ERROR USG (H7)        #");
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

    /* 7. Estado físico de seguridad (LEDs) */
    // Para el UsageFault usamos parpadeo de advertencia o combinación distinta a las anteriores
    configErrorLed.mode = MODE_ONLY_GREEN; 
    configErrorLed.delayMs = BLINK_OFF;
    configErrorLed.modePanic = true; 
    configErrorLed.modePanicTimeOut = BLINK_PANIC_OFF;
    applyLedConfiguration(&configErrorLed);
    

    /* 8. Bloqueo seguro del procesador */
    __disable_irq();

    while (1) 
    {
        applyLedConfiguration(&configErrorLed);
    }
}


/**
 * @brief  Manejador de excepción de BusFault en ensamblador.
 * @note   Detecta qué puntero de pila (MSP o PSP) estaba activo cuando colapsó 
 *         el bus de la CPU y se lo pasa como argumento a la función en C.
 */
__attribute__((naked)) void BusFault_Handler(void)
{
    __asm volatile
    (
        "tst lr, #4                        \n" /* Comprueba el bit 2 de EXEC_RETURN (LR) */
        "ite eq                            \n" /* Si es 0, el sistema usaba MSP. Si es 1, PSP */
        "mrseq r0, msp                     \n" /* R0 = Main Stack Pointer */
        "mrsne r0, psp                     \n" /* R0 = Process Stack Pointer */
        "b BusFault_Handler_C              \n" /* Salta a la función en C pasando R0 como parámetro */
    );
}

/**
 * @brief  Procesador del BusFault en C.
 * @param  stack Puntero al bloque de la pila salvado automáticamente por la CPU.
 */
void BusFault_Handler_C(uint32_t *stack)
{
    /* 1. Volcado de los registros básicos guardados en la pila */
    uint32_t r0  = stack[0];
    uint32_t r1  = stack[1];
    uint32_t r2  = stack[2];
    uint32_t r3  = stack[3];
    uint32_t r12 = stack[4];
    uint32_t lr  = stack[5];
    uint32_t pc  = stack[6];
    uint32_t psr = stack[7];

    /* 2. Captura de los registros de diagnóstico del SCB */
    uint32_t cfsr  = SCB->CFSR;
    uint32_t hfsr  = SCB->HFSR;
    uint32_t mmfar = SCB->MMFAR;
    uint32_t bfar  = SCB->BFAR;

    /* 3. Rellenamos tu estructura global ErrorHandler */
    ErrorHandler.file     = "BUSFAULT";
    ErrorHandler.function = "BusFault_Handler_C";
    ErrorHandler.line     = pc; 
    ErrorHandler.err_code = ERR_HANDLER_BUSFAULT; 
    ErrorHandler.err_name = "BUS_FAULT";

    /* 4. Análisis dinámico del registro CFSR (sección BusFault) para escribir la descripción */
    // El subregistro BFSR comparte espacio dentro de CFSR (bits 8 al 15)
    if (cfsr & SCB_CFSR_IBUSERR_Msk) 
    {
        ErrorHandler.description = "Fallo de Bus: Error en bus de instrucciones al intentar pre-cargar una instruccion (IBUSERR)";
    } 
    else if (cfsr & SCB_CFSR_PRECISERR_Msk) 
    {
        ErrorHandler.description = "Fallo de Bus: Error de datos preciso. Operacion de lectura/escritura ilegal (PRECISERR)";
    } 
    else if (cfsr & SCB_CFSR_IMPRECISERR_Msk) 
    {
        ErrorHandler.description = "Fallo de Bus: Error de datos impreciso. Retraso en el bus de datos (IMPRECISERR)";
    } 
    else if (cfsr & SCB_CFSR_UNSTKERR_Msk) 
    {
        ErrorHandler.description = "Fallo de Bus: Error al intentar sacar registros de la pila al retornar de una excepcion (UNSTKERR)";
    } 
    else if (cfsr & SCB_CFSR_STKERR_Msk) 
    {
        ErrorHandler.description = "Fallo de Bus: Error al intentar meter registros en la pila al entrar a una excepcion (STKERR)";
    } 
    else 
    {
        ErrorHandler.description = "Fallo de Bus: Error general de acceso a periferico o memoria invalida en el bus del STM32H7";
    }

    /* Adicionalmente, si el bit BFARVALID está activo, significa que la dirección errónea quedó grabada en el registro BFAR */
    if (cfsr & SCB_CFSR_BFARVALID_Msk)
    {
        // Añadimos la dirección física conflictiva al final de la descripción
        static char desc_con_direccion[128];
        snprintf(desc_con_direccion, sizeof(desc_con_direccion), "%s [Direccion del fallo: 0x%08lX]", ErrorHandler.description, bfar);
        ErrorHandler.description = desc_con_direccion;
    }

    /* 5. Estampa de tiempo y lectura del calendario RTC */
    ErrorHandler.timestamp = HAL_GetTick();

    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK)
    {
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    }

    /* 6. Diagnóstico por puerto serie con tu formato exacto de banners */
    printf("\r\n###########################################################");
    printf("\r\n#                PÁNICO DEL SISTEMA ERROR BUS (H7)        #");
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

    /* 7. Estado físico de seguridad (LEDs) */
    // Configuramos combinación de Rojo + Amarillo Fijos (así difiere de los anteriores)
    configErrorLed.mode = MODE_RED_BLINK; 
    configErrorLed.delayMs = BLINK_OFF;
    configErrorLed.modePanic = true; 
    configErrorLed.modePanicTimeOut = BLINK_PANIC_NO_SATURE;
    applyLedConfiguration(&configErrorLed);

    /* 8. Bloqueo seguro del procesador */
    __disable_irq();

    while (1) 
    {
        applyLedConfiguration(&configErrorLed);
    }
}


/**
 * @brief  Manejador de excepción de MemManage (MPU) en ensamblador.
 * @note   Utiliza el atributo 'naked' para evitar que el compilador altere la pila.
 *         Captura el SP activo (MSP o PSP) y salta a la interpretación en C.
 */
__attribute__((naked)) void MemManage_Handler(void)
{
    __asm volatile
    (
        "tst lr, #4                        \n" /* Comprueba el bit 2 de EXEC_RETURN (LR) */
        "ite eq                            \n" /* Si es 0, el sistema usaba MSP. Si es 1, PSP */
        "mrseq r0, msp                     \n" /* R0 = Main Stack Pointer */
        "mrsne r0, psp                     \n" /* R0 = Process Stack Pointer */
        "b MemManage_Handler_C             \n" /* Salta a la función en C pasando R0 como parámetro */
    );
}

/**
 * @brief  Procesador del MemManage Fault en C.
 * @param  stack Puntero al bloque de la pila salvado de forma intacta por la CPU.
 */
void MemManage_Handler_C(uint32_t *stack)
{
    /* 1. Volcado inmediato de los registros básicos guardados en la pila */
    uint32_t r0  = stack[0];
    uint32_t r1  = stack[1];
    uint32_t r2  = stack[2];
    uint32_t r3  = stack[3];
    uint32_t r12 = stack[4];
    uint32_t lr  = stack[5];
    uint32_t pc  = stack[6];
    uint32_t psr = stack[7];

    /* 2. Captura de los registros de diagnóstico del System Control Block (SCB) */
    uint32_t cfsr  = SCB->CFSR;
    uint32_t hfsr  = SCB->HFSR;
    uint32_t mmfar = SCB->MMFAR; // Registro específico con la dirección que violó la MPU
    uint32_t bfar  = SCB->BFAR;

    /* 3. Rellenamos la estructura global de tu "Caja Negra" */
    ErrorHandler.file     = "MEMMANAGE";
    ErrorHandler.function = "MemManage_Handler_C";
    ErrorHandler.line     = pc; 
    ErrorHandler.err_code = ERR_HANDLER_MEMMANAGE; 
    ErrorHandler.err_name = "MEMMANAGE_FAULT";

    /* 4. Análisis dinámico del registro CFSR (sección MMFSR, bits 0 al 7) */
    if (cfsr & SCB_CFSR_IACCVIOL_Msk) 
    {
        ErrorHandler.description = "Fallo de Memoria: Violacion de acceso a instruccion. Intento de ejecutar codigo desde una region prohibida (IACCVIOL)";
    } 
    else if (cfsr & SCB_CFSR_DACCVIOL_Msk) 
    {
        ErrorHandler.description = "Fallo de Memoria: Violacion de acceso a datos. Intento de leer o escribir en una region prohibida (DACCVIOL)";
    } 
    else if (cfsr & SCB_CFSR_MUNSTKERR_Msk) 
    {
        ErrorHandler.description = "Fallo de Memoria: Error de la MPU al intentar sacar registros de la pila (MUNSTKERR)";
    } 
    else if (cfsr & SCB_CFSR_MSTKERR_Msk) 
    {
        ErrorHandler.description = "Fallo de Memoria: Error de la MPU al intentar meter registros en la pila (MSTKERR)";
    } 
    else if (cfsr & SCB_CFSR_MLSPERR_Msk) 
    {
        ErrorHandler.description = "Fallo de Memoria: Error de la MPU durante el salvado perezoso del contexto de punto flotante FPU (MLSPERR)";
    } 
    else 
    {
        ErrorHandler.description = "Fallo de Memoria: Violacion de acceso o proteccion de memoria gestionada por la MPU en el STM32H7";
    }

    /* Si la dirección de memoria que causó la violación es válida, la extraemos del registro MMFAR */
    if (cfsr & SCB_CFSR_MMARVALID_Msk)
    {
        static char desc_con_direccion[128];
        snprintf(desc_con_direccion, sizeof(desc_con_direccion), "%s [Direccion violada: 0x%08lX]", ErrorHandler.description, mmfar);
        ErrorHandler.description = desc_con_direccion;
    }

    /* 5. Estampa de tiempo y lectura del calendario RTC */
    ErrorHandler.timestamp = HAL_GetTick();

    RTC_TimeTypeDef sTime = {0};
    RTC_DateTypeDef sDate = {0};

    if (HAL_RTC_GetTime(&hrtc, &sTime, RTC_FORMAT_BIN) == HAL_OK)
    {
        HAL_RTC_GetDate(&hrtc, &sDate, RTC_FORMAT_BIN);
    }

    /* 6. Diagnóstico por puerto serie con tu formato exacto de banners */
    printf("\r\n###########################################################");
    printf("\r\n#                PÁNICO DEL SISTEMA ERROR MEM (H7)        #");
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

    /* 7. Estado físico de seguridad (LEDs) */
    // Activamos combinación de LED Rojo + LED Verde Fijos (así difiere visualmente del resto)
    configErrorLed.mode = MODE_YELLOW_BLINK; 
    configErrorLed.delayMs = BLINK_OFF;
    configErrorLed.modePanic = true; 
    configErrorLed.modePanicTimeOut = BLINK_PANIC_NO_SATURE;
    applyLedConfiguration(&configErrorLed);

    /* 8. Bloqueo seguro del procesador */
    __disable_irq();

    while (1) 
    {
        applyLedConfiguration(&configErrorLed);
    }
}