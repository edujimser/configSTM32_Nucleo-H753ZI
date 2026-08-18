#include "MsgUart/TxUart/TxUart.h"
#include <string.h>
#include "main.h"
#include <stdint.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "usart.h"
#include "stm32h7xx_hal_uart.h"
#include "MsgUart/TestUart/TestUart.h"

// ==========================================================================================================================================
// INSTANCIA
// ==========================================================================================================================================
struct TestUartInstance {
	E_Status_TestUart Status;
	bool Activacion;

};

I_TestUart TestUart_Init(bool Activacion){
	struct TestUartInstance  *TestUart = malloc(sizeof(struct TestUartInstance ));

	if (TestUart != NULL) {
		TestUart->Status = ESTADO_PARADA;
		TestUart->Activacion = Activacion;
	}

	return TestUart;
}


// ==========================================================================================================================================
// FUNCIONES INSTANCIA
// ==========================================================================================================================================
//GET -->
E_Status_TestUart TestUart_GetStatus(I_TestUart TestUart) {
    if (TestUart == NULL) {return ESTADO_PARADA;}
    return TestUart->Status;
}

bool TestUart_GetActivacion(I_TestUart TestUart){
	if (TestUart == NULL) {return false;};
	return TestUart->Activacion;
};


//SET -->
void TestUart_SetStatus(I_TestUart TestUart, E_Status_TestUart Status) {

    if (TestUart == NULL) {return;}
    TestUart->Status = Status;
}

void TestUart_SetActivacion(I_TestUart TestUart, bool Activacion) {

    if (TestUart == NULL) {return;}
    TestUart->Activacion = Activacion;
}














// ==========================================================================================================================================
// FUNCIONES
// ==========================================================================================================================================
void Prueba_TX(I_TestUart TestUart, bool Activacion)
{
	if(TestUart == NULL){
		return;
	}

	TestUart_SetActivacion(TestUart, Activacion);

	if(TestUart_GetActivacion(TestUart)){
		TestUart_SetStatus(TestUart, ESTADO_CORRIENDO);
	}

	if (TestUart->Activacion && (TestUart->Status == ESTADO_CORRIENDO))
	{
	    // 1. Cabecera general de la batería completa
	    MSG_Enviar_Test("\r\n\r\n");
	    MSG_Enviar_Test("########################################################\r\n");
	    MSG_Enviar_Test("###     INICIANDO BATERIA DE PRUEBAS DE ROBUSTEZ     ###\r\n");
	    MSG_Enviar_Test("########################################################\r\n");

	    // Pausa inicial para que el monitor serie procese la bienvenida
	    vTaskDelay(pdMS_TO_TICKS(2000));

	    // 1) Longitud máxima superada
	    Prueba_Longitud_Maxima();
	    vTaskDelay(pdMS_TO_TICKS(3000));

	    // 2) Mensajes vacíos y NULL
	    Prueba_Mensajes_Vacios();
	    vTaskDelay(pdMS_TO_TICKS(3000));

	    // 3) Caracteres inválidos
	    Prueba_Caracteres_Invalidos();
	    vTaskDelay(pdMS_TO_TICKS(3000));

	    // 4) Saturación de cola
	    Prueba_Saturacion_Cola();
	    vTaskDelay(pdMS_TO_TICKS(3000));

	    // 5) Condición de carrera entre tareas
	    PruebaConcurrente_Tiempo(20000);
	    vTaskDelay(pdMS_TO_TICKS(3000));

	    // 6) Abortos DMA repetidos
	    Prueba_Abortos_DMA_Repetidos();
	    vTaskDelay(pdMS_TO_TICKS(2000));

	    // 2. Cierre general de la batería completa
	    MSG_Enviar_Test("\r\n");
	    MSG_Enviar_Test("########################################################\r\n");
	    MSG_Enviar_Test("###   BATERIA DE PRUEBAS FINALIZADA CORRECTAMENTE    ###\r\n");
	    MSG_Enviar_Test("########################################################\r\n\r\n");
	}

	TestUart_SetActivacion(TestUart, false);

	if(!TestUart_GetActivacion(TestUart)){
	    TestUart_SetStatus(TestUart, ESTADO_PARADA);
	}
}

// =======================
// 1) PRUEBA LONGITUD MÁXIMA
// =======================
void Prueba_Longitud_Maxima(void)
{
    // 1. Cabecera visual clara
    MSG_Enviar_Test("\r\n========================================================\r\n");
    MSG_Enviar_Test("[TEST 1] INICIO: Prueba de Longitud Maxima\r\n");
    MSG_Enviar_Test("========================================================\r\n");

    // 2. Contexto de lo que va a ocurrir
    MSG_Enviar_Test(">> Info: Generando un buffer gigante de 199 caracteres...\r\n");
    MSG_Enviar_Test(">> Esperado: El sistema debe rechazar el mensaje por seguridad.\r\n");
    MSG_Enviar_Test(">> Esperado: Debes ver un mensaje de error indicando el limite.\r\n");
    MSG_Enviar_Test("--------------------------------------------------------\r\n");

    // Damos tiempo a la tarea UART para enviar la cabecera antes del impacto
    vTaskDelay(pdMS_TO_TICKS(100));

    // 3. Ejecución de la prueba
    char bufferLargo[200];
    memset(bufferLargo, 'A', sizeof(bufferLargo) - 1);
    bufferLargo[sizeof(bufferLargo) - 1] = '\0';

    MSG_Enviar_Test(bufferLargo);

    // Damos tiempo suficiente para que el sistema procese y rechace el mensaje
    vTaskDelay(pdMS_TO_TICKS(500));

    // 4. Cierre visual de la prueba
    MSG_Enviar_Test("--------------------------------------------------------\r\n");
    MSG_Enviar_Test("[TEST 1] FIN: Evaluacion completada.\r\n");
    MSG_Enviar_Test("========================================================\r\n");
}

// =======================
// 2) PRUEBA MENSAJES VACÍOS / NULL
// =======================
void Prueba_Mensajes_Vacios(void)
{
    // 1. Cabecera visual clara
    MSG_Enviar_Test("\r\n========================================================\r\n");
    MSG_Enviar_Test("[TEST 2] INICIO: Prueba de Mensajes Vacios y NULL\r\n");
    MSG_Enviar_Test("========================================================\r\n");

    // 2. Contexto de lo que va a ocurrir
    MSG_Enviar_Test(">> Info: Inyectando una cadena vacia (\"\") y un puntero NULL...\r\n");
    MSG_Enviar_Test(">> Esperado: El sistema debe descartarlos internamente por seguridad.\r\n");
    MSG_Enviar_Test(">> Esperado: El microcontrolador NO debe colgarse (HardFault).\r\n");
    MSG_Enviar_Test("--------------------------------------------------------\r\n");

    // Damos tiempo para imprimir la cabecera
    vTaskDelay(pdMS_TO_TICKS(100));

    // 3. Ejecución de la prueba
    MSG_Enviar_Test("");        // Prueba 1: Texto vacío
    vTaskDelay(pdMS_TO_TICKS(100));

    MSG_Enviar_Test(NULL);      // Prueba 2: Puntero nulo
    vTaskDelay(pdMS_TO_TICKS(500));

    // 4. Cierre visual de la prueba
    MSG_Enviar_Test("--------------------------------------------------------\r\n");
    MSG_Enviar_Test("[TEST 2] FIN: Evaluacion completada.\r\n");
    MSG_Enviar_Test("========================================================\r\n");
}

// =======================
// 3) PRUEBA CARACTERES INVÁLIDOS
// =======================
void Prueba_Caracteres_Invalidos(void)
{
    // 1. Cabecera visual clara
    MSG_Enviar_Test("\r\n========================================================\r\n");
    MSG_Enviar_Test("[TEST 3] INICIO: Prueba de Caracteres Invalidos\r\n");
    MSG_Enviar_Test("========================================================\r\n");

    // 2. Contexto de lo que va a ocurrir
    MSG_Enviar_Test(">> Info: Inyectando caracteres de control no imprimibles (0x01, 0x02)...\r\n");
    MSG_Enviar_Test(">> Esperado: El filtro de la UART debe rechazar la cadena para proteger el terminal.\r\n");
    MSG_Enviar_Test(">> Esperado: Debes ver un mensaje de error por caracter fuera de rango.\r\n");
    MSG_Enviar_Test("--------------------------------------------------------\r\n");

    // Damos tiempo a imprimir la cabecera antes de lanzar la prueba
    vTaskDelay(pdMS_TO_TICKS(100));

    // 3. Ejecución de la prueba
    char bufferInvalidos[32];
    bufferInvalidos[0] = 0x01; // control no permitido
    bufferInvalidos[1] = 0x02; // control no permitido
    bufferInvalidos[2] = '\r'; // permitido
    bufferInvalidos[3] = '\n'; // permitido
    bufferInvalidos[4] = '\0'; // terminador

    MSG_Enviar_Test(bufferInvalidos);  // Debe activar MSG_Enviar_Error()

    // Tiempo para que la cola procese y el mensaje de error se envíe
    vTaskDelay(pdMS_TO_TICKS(500));

    // 4. Cierre visual de la prueba
    MSG_Enviar_Test("--------------------------------------------------------\r\n");
    MSG_Enviar_Test("[TEST 3] FIN: Evaluacion completada.\r\n");
    MSG_Enviar_Test("========================================================\r\n");
}

// =======================
// 4) PRUEBA SATURACIÓN DE COLA
// =======================
void Prueba_Saturacion_Cola(void)
{
    // 1. Cabecera visual clara
    MSG_Enviar_Test("\r\n========================================================\r\n");
    MSG_Enviar_Test("[TEST 4] INICIO: Prueba de Saturacion de Cola\r\n");
    MSG_Enviar_Test("========================================================\r\n");

    // 2. Contexto de lo que va a ocurrir
    MSG_Enviar_Test(">> Info: Bombardeando la cola con 200 mensajes en 200 ms...\r\n");
    MSG_Enviar_Test(">> Esperado: La cola se llenara. Los ultimos mensajes seran rechazados.\r\n");
    MSG_Enviar_Test(">> Esperado: El sistema debe liberar la memoria de los mensajes caidos (No Memory Leaks).\r\n");
    MSG_Enviar_Test(">> Esperado: Tras vaciarse, el sistema debe recuperarse y seguir transmitiendo.\r\n");
    MSG_Enviar_Test("--------------------------------------------------------\r\n");

    // Damos tiempo a imprimir la cabecera
    vTaskDelay(pdMS_TO_TICKS(100));

    // 3. Ejecución de la prueba (Saturación)
    for (int i = 0; i < 200; i++)
    {
    	// Se envían mucho más rápido de lo que la UART puede sacarlos
    	MSG_Enviar_Test("Mensaje de relleno de cola...\r\n");
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    // 4. Intento de desbordamiento (La cola ya debería estar al 100%)
    // Estos mensajes probablemente NO saldrán por pantalla, caerán en la ruta de error internamente
    MSG_Enviar_Test(">>> ERROR FORZADO: Intentando enviar con cola llena <<<\r\n");
    MSG_Enviar_Test(">>> ERROR FORZADO: Mensaje con cola llena <<<\r\n");

    // 5. Recuperación
    // Necesitamos darle tiempo suficiente a la UART para que transmita los 200 mensajes
    // y vacíe la cola, o de lo contrario el mensaje final también se descartará.
    vTaskDelay(pdMS_TO_TICKS(1500));

    MSG_Enviar_Test(">> INFO: La cola deberia haberse vaciado y el sistema recuperado correctamente.\r\n");
    vTaskDelay(pdMS_TO_TICKS(1000));

    // 6. Cierre visual de la prueba
    MSG_Enviar_Test("\r\n--------------------------------------------------------\r\n");
    MSG_Enviar_Test("[TEST 4] FIN: Evaluacion completada.\r\n");
    MSG_Enviar_Test("========================================================\r\n");
}

// =======================
// 5) PRUEBA CONCURRENCIA ENTRE TAREAS
// =======================
void PruebaConcurrente_Tiempo(uint32_t tiempo_ms)
{
// 1. Cabecera visual clara
	MSG_Enviar_Test("\r\n========================================================\r\n");
	MSG_Enviar_Test("[TEST 5] INICIO: Prueba Concurrente de Carga Continua\r\n");
	MSG_Enviar_Test("========================================================\r\n");

	// 2. Contexto de lo que va a ocurrir (TEXTO FIJO, SIN SNPRINTF)
	MSG_Enviar_Test(">> Info: Inyectando mensajes continuamente durante 20 segundos...\r\n");
	MSG_Enviar_Test(">> Esperado: La UART y FreeRTOS deben mantener la estabilidad bajo carga constante.\r\n");
	MSG_Enviar_Test(">> Esperado: No deben ocurrir bloqueos del microcontrolador.\r\n");
	MSG_Enviar_Test("--------------------------------------------------------\r\n");

    // Damos tiempo a imprimir la cabecera
    vTaskDelay(pdMS_TO_TICKS(100));

    // 3. Ejecución de la prueba
    uint32_t inicio = HAL_GetTick();

    while ((HAL_GetTick() - inicio) < tiempo_ms)
    {
    	MSG_Enviar_Test(">> Carga concurrente: validando estabilidad RTOS...\r\n");

        // Usamos vTaskDelay en lugar de HAL_Delay para permitir
        // que el RTOS cambie de contexto a la tarea de la UART
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Damos tiempo a que se vacíe la cola tras el intenso bombardeo
    // (Si fueron 20000 ms, la cola estará muy llena)
    vTaskDelay(pdMS_TO_TICKS(1000));

    // 4. Cierre visual de la prueba
    MSG_Enviar_Test("--------------------------------------------------------\r\n");
    MSG_Enviar_Test("[TEST 5] FIN: Evaluacion concurrente completada con exito.\r\n");
    MSG_Enviar_Test("========================================================\r\n");
}

// =======================
// 6) PRUEBA ABORTOS DMA REPETIDOS
// =======================
void Prueba_Abortos_DMA_Repetidos(void)
{
    // 1. Cabecera visual clara
    MSG_Enviar_Test("\r\n========================================================\r\n");
    MSG_Enviar_Test("[TEST 6] INICIO: Prueba de Abortos DMA Repetidos\r\n");
    MSG_Enviar_Test("========================================================\r\n");

    // 2. Contexto de lo que va a ocurrir
    MSG_Enviar_Test(">> Info: Iniciando y abortando transferencias DMA 20 veces rapidas...\r\n");
    MSG_Enviar_Test(">> Esperado: El driver HAL y el hardware DMA deben soportar los cortes abruptos.\r\n");
    MSG_Enviar_Test(">> Esperado: Tras la prueba, la UART debe recuperarse y poder imprimir este fin de prueba.\r\n");
    MSG_Enviar_Test("--------------------------------------------------------\r\n");

    // Damos tiempo a la cabecera para que se envíe sin interrupciones
    vTaskDelay(pdMS_TO_TICKS(100));

    // 3. Ejecución de la prueba
    // OBLIGATORIO: Alinear a 32 bytes para no romper la caché
    ALIGN_32BYTES(static char buffer[64]);
    strcpy(buffer, "Mensaje DMA para abortar repetidamente\r\n");

    for (int i = 0; i < 20; i++)
    {
        // Limpiamos caché (tamaño 64 es múltiplo de 32, así que es seguro)
        SCB_CleanDCache_by_Addr((uint32_t*)buffer, 64);

        // Si la UART está lista, disparamos
        if (HAL_UART_GetState(&huart3) == HAL_UART_STATE_READY)
        {
            HAL_UART_Transmit_DMA(&huart3, (uint8_t*)buffer, strlen(buffer));

            // Dejamos que el DMA empiece su trabajo 1ms...
            vTaskDelay(pdMS_TO_TICKS(1));

            // ¡Hachazo! Cortamos la transmisión a medias
            HAL_UART_AbortTransmit(&huart3);
        }
        else
        {
            // Si se quedó trabada del ciclo anterior, forzamos un aborto preventivo
            HAL_UART_AbortTransmit(&huart3);
        }

        vTaskDelay(pdMS_TO_TICKS(10));
    }

    // Damos tiempo extra al HAL para asentar su estado interno (gState)
    vTaskDelay(pdMS_TO_TICKS(100));

    // 4. Cierre visual de la prueba
    MSG_Enviar_Test("--------------------------------------------------------\r\n");
    MSG_Enviar_Test("[TEST 6] FIN: Evaluacion de hardware completada.\r\n");
    MSG_Enviar_Test("========================================================\r\n");
}

