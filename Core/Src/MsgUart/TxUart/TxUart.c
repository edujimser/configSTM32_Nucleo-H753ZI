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
// TRANSMISIÓNM
// ==========================================================================================================================================

uint32_t Stack_libre_uarth;
uint32_t Stack_libre_uarth_critic;
extern I_TestUart TestUart_1;

osMessageQueueId_t hUartQueue = NULL;
osThreadId_t UartTaskHandle = NULL;

void MSG_Enviar(const char *texto, I_TestUart TestUart) {
	if(TestUart_GetStatus(TestUart) == ESTADO_CORRIENDO) return;

    // 1. Comprobación: el kernel debe estar en ejecución.
    //    Si FreeRTOS aún no ha arrancado, no podemos usar colas.
    if (osKernelGetState() != osKernelRunning) return;

    // 2. Comprobación: la cola debe existir.
    //    Si la cola no está creada, no enviamos nada.
    if (hUartQueue == NULL) return;

    // 3. Comprobación: texto válido.
    //    Evita punteros nulos y fallos de memoria.
    if (texto == NULL) return;

    // 4. Validación de longitud máxima definida por el usuario.
    //    Evita mensajes demasiado grandes que saturen el heap o DMA.
    if (MSG_Enviar_Longitud_Limit(texto)) {
        MSG_Enviar_Error("Se ha superado el limite maximo de caracteres \r\n");
        return;
    }

    // 5. Validación de caracteres permitidos.
    //    Evita caracteres de control no imprimibles (excepto \n y \r).
    if (MSG_Enviar_Caract_Valid(texto)) {
        MSG_Enviar_Error("Se ha introducido un carcacter fuera de rango \r\n");
        return;
    }

    // ---------------------------------------------------------
    // RESERVA DE LA ESTRUCTURA EN EL HEAP
    // ---------------------------------------------------------
    // Creamos un puntero a estructura S_UartMsg.
    // IMPORTANTE: la estructura se reserva en heap, no en la pila.
    // Esto permite que sobreviva después de salir de esta función.
    S_UartMsg *msg = pvPortMalloc(sizeof(S_UartMsg));
    if (msg == NULL) return;   // Seguridad ante falta de memoria

    // ---------------------------------------------------------
    // RESERVA DEL BUFFER DE TEXTO EN EL HEAP
    // ---------------------------------------------------------
    // Calculamos la longitud del texto (+1 para el terminador '\0').
    msg->len = strlen(texto) + 1;

    // Reservamos memoria para el texto.
    msg->Ptr = pvPortMalloc(msg->len);
    if (msg->Ptr == NULL) {
        // Si falla la reserva del texto, liberamos la estructura.
        vPortFree(msg);
        return;
    }

    // ---------------------------------------------------------
    // COPIA SEGURA DEL TEXTO
    // ---------------------------------------------------------
    // Copiamos el texto al buffer reservado.
    // strncpy evita desbordamientos.
    strncpy(msg->Ptr, texto, msg->len - 1);

    // Aseguramos que el último byte sea '\0'.
    msg->Ptr[msg->len - 1] = '\0';

    // ---------------------------------------------------------
    // ENVÍO DEL MENSAJE A LA COLA
    // ---------------------------------------------------------
    // Enviamos *el puntero a la estructura* (S_UartMsg *) a la cola.
    // La cola está configurada para almacenar punteros (sizeof(S_UartMsg *)).
    if (osMessageQueuePut(hUartQueue, &msg, 0, UART_QUEUE_TIMEOUT_TICKS) != osOK) {

        // Si la cola está llena o falla el envío:
        // liberamos el texto y la estructura para evitar fugas de memoria.
        vPortFree(msg->Ptr);
        vPortFree(msg);
    }
}

void MSG_Enviar_Test(const char *texto) {

    // 1. Comprobación: el kernel debe estar en ejecución.
    //    Si FreeRTOS aún no ha arrancado, no podemos usar colas.
    if (osKernelGetState() != osKernelRunning) return;

    // 2. Comprobación: la cola debe existir.
    //    Si la cola no está creada, no enviamos nada.
    if (hUartQueue == NULL) return;

    // 3. Comprobación: texto válido.
    //    Evita punteros nulos y fallos de memoria.
    if (texto == NULL) return;

    // 4. Validación de longitud máxima definida por el usuario.
    //    Evita mensajes demasiado grandes que saturen el heap o DMA.
    if (MSG_Enviar_Longitud_Limit(texto)) {
        MSG_Enviar_Error("Se ha superado el limite maximo de caracteres \r\n");
        return;
    }

    // 5. Validación de caracteres permitidos.
    //    Evita caracteres de control no imprimibles (excepto \n y \r).
    if (MSG_Enviar_Caract_Valid(texto)) {
        MSG_Enviar_Error("Se ha introducido un carcacter fuera de rango \r\n");
        return;
    }

    // ---------------------------------------------------------
    // RESERVA DE LA ESTRUCTURA EN EL HEAP
    // ---------------------------------------------------------
    // Creamos un puntero a estructura S_UartMsg.
    // IMPORTANTE: la estructura se reserva en heap, no en la pila.
    // Esto permite que sobreviva después de salir de esta función.
    S_UartMsg *msg = pvPortMalloc(sizeof(S_UartMsg));
    if (msg == NULL) return;   // Seguridad ante falta de memoria

    // ---------------------------------------------------------
    // RESERVA DEL BUFFER DE TEXTO EN EL HEAP
    // ---------------------------------------------------------
    // Calculamos la longitud del texto (+1 para el terminador '\0').
    msg->len = strlen(texto) + 1;

    // Reservamos memoria para el texto.
    msg->Ptr = pvPortMalloc(msg->len);
    if (msg->Ptr == NULL) {
        // Si falla la reserva del texto, liberamos la estructura.
        vPortFree(msg);
        return;
    }

    // ---------------------------------------------------------
    // COPIA SEGURA DEL TEXTO
    // ---------------------------------------------------------
    // Copiamos el texto al buffer reservado.
    // strncpy evita desbordamientos.
    strncpy(msg->Ptr, texto, msg->len - 1);

    // Aseguramos que el último byte sea '\0'.
    msg->Ptr[msg->len - 1] = '\0';

    // ---------------------------------------------------------
    // ENVÍO DEL MENSAJE A LA COLA
    // ---------------------------------------------------------
    // Enviamos *el puntero a la estructura* (S_UartMsg *) a la cola.
    // La cola está configurada para almacenar punteros (sizeof(S_UartMsg *)).
    if (osMessageQueuePut(hUartQueue, &msg, 0, UART_QUEUE_TIMEOUT_TICKS) != osOK) {

        // Si la cola está llena o falla el envío:
        // liberamos el texto y la estructura para evitar fugas de memoria.
        vPortFree(msg->Ptr);
        vPortFree(msg);
    }
}

void MSG_Enviar_Error(const char *texto) {
    // 1. Comprobación: el kernel debe estar en ejecución.
    //    Si FreeRTOS aún no ha arrancado, no podemos usar colas.
    if (osKernelGetState() != osKernelRunning) return;

    // 2. Comprobación: la cola debe existir.
    //    Si la cola no está creada, no enviamos nada.
    if (hUartQueue == NULL) return;

    // 3. Comprobación: texto válido.
    //    Evita punteros nulos y fallos de memoria.
    if (texto == NULL) return;

    // ---------------------------------------------------------
	// RESERVA DE LA ESTRUCTURA EN EL HEAP
	// ---------------------------------------------------------
	// Creamos un puntero a estructura S_UartMsg.
	// IMPORTANTE: la estructura se reserva en heap, no en la pila.
	// Esto permite que sobreviva después de salir de esta función.
	S_UartMsg *msg = pvPortMalloc(sizeof(S_UartMsg));
	if (msg == NULL) return;   // Seguridad ante falta de memoria

    // ---------------------------------------------------------
	// RESERVA DEL BUFFER DE TEXTO EN EL HEAP
	// ---------------------------------------------------------
	// Calculamos la longitud del texto (+1 para el terminador '\0').
	msg->len = strlen(texto) + 1;

	// Reservamos memoria para el texto.
	msg->Ptr = pvPortMalloc(msg->len);
	if (msg->Ptr == NULL) {
		// Si falla la reserva del texto, liberamos la estructura.
		vPortFree(msg);
		return;
	}

	// ---------------------------------------------------------
	// COPIA SEGURA DEL TEXTO
	// ---------------------------------------------------------
	// Copiamos el texto al buffer reservado.
	// strncpy evita desbordamientos.
	strncpy(msg->Ptr, texto, msg->len - 1);

	// Aseguramos que el último byte sea '\0'.
	msg->Ptr[msg->len - 1] = '\0';

	// ---------------------------------------------------------
	// ENVÍO DEL MENSAJE A LA COLA
	// ---------------------------------------------------------
	// Enviamos *el puntero a la estructura* (S_UartMsg *) a la cola.
	// La cola está configurada para almacenar punteros (sizeof(S_UartMsg *)).
	if (osMessageQueuePut(hUartQueue, &msg, 0, UART_QUEUE_TIMEOUT_TICKS) != osOK) {

		// Si la cola está llena o falla el envío:
		// liberamos el texto y la estructura para evitar fugas de memoria.
		vPortFree(msg->Ptr);
		vPortFree(msg);
	}
}

char MSG_Enviar_Longitud_Limit(const char *texto){

	size_t len = strlen(texto) + 1;

	if(len > LONGITUD_MAX_UART){
		return true;
	}
	 return false;
}

char MSG_Enviar_Caract_Valid(const char *texto){

    for (size_t i = 0; i < strlen(texto); i++) {
        if ((unsigned char)texto[i] < 32 && texto[i] != '\n' && texto[i] != '\r') {
        	return true;
        }
    }
    return false;
}

void vTaskUartHandler(void *argument) {
    S_UartMsg *msg ;

    for (;;) {

        Stack_libre_uarth = osThreadGetStackSpace(osThreadGetId());

        if (osMessageQueueGet(hUartQueue, &msg, NULL, osWaitForever) == osOK) {

        	char *msgPtr = msg->Ptr;
        	uint32_t len = msg->len;

            if (msgPtr != NULL) {

                if (len == 0) {
                    vPortFree(msgPtr);
                    msgPtr = NULL;

                	vPortFree(msg);
                	msg = NULL;

                    continue;
                }

                SCB_CleanDCache_by_Addr((uint32_t*)msgPtr, len);

                HAL_UART_StateTypeDef st = HAL_UART_GetState(&huart3);

                switch (st) {

                    case HAL_UART_STATE_READY:

                        osThreadFlagsClear(0x0001U);

                        if (HAL_UART_Transmit_DMA(&huart3, (uint8_t *)msgPtr, len) == HAL_OK) {

                            Stack_libre_uarth_critic = osThreadGetStackSpace(osThreadGetId());

                            uint32_t flags = osThreadFlagsWait(0x0001U, osFlagsWaitAny, 100);

                            if (flags == osFlagsErrorTimeout) {
                            	huart3.gState = HAL_UART_STATE_READY;
                                HAL_UART_AbortTransmit(&huart3);
                            }
                        }else {
                            // CORRECCIÓN 3: Si HAL_UART_Transmit_DMA no devuelve HAL_OK, liberar gState
                            HAL_UART_AbortTransmit_IT(&huart3);
                            huart3.gState = HAL_UART_STATE_READY;
                        }

                        osThreadFlagsClear(0x0001U);
                        break;

                    case HAL_UART_STATE_BUSY:
                    case HAL_UART_STATE_BUSY_TX:
                    case HAL_UART_STATE_BUSY_RX:

                    	osThreadFlagsClear(0x0001U);
						if (HAL_UART_Transmit_DMA(&huart3, (uint8_t *)msgPtr, len) == HAL_OK) {
							uint32_t flags = osThreadFlagsWait(0x0001U, osFlagsWaitAny, 200);
							if (flags == osFlagsErrorTimeout) {
								HAL_UART_AbortTransmit_IT(&huart3);
								huart3.gState = HAL_UART_STATE_READY;
							}
						} else {
							HAL_UART_AbortTransmit_IT(&huart3);
							huart3.gState = HAL_UART_STATE_READY;
						}
						osThreadFlagsClear(0x0001U);
						break;
                    case HAL_UART_STATE_BUSY_TX_RX:

                        osThreadFlagsClear(0x0001U);
                        HAL_UART_AbortTransmit_IT(&huart3);
                        huart3.gState = HAL_UART_STATE_READY;
                        break;

                    case HAL_UART_STATE_TIMEOUT:

                        osThreadFlagsClear(0x0001U);
                        HAL_UART_AbortTransmit(&huart3);
                        osThreadFlagsClear(0x0001U);
                        break;

                    case HAL_UART_STATE_RESET:
                    case HAL_UART_STATE_ERROR:

                        osThreadFlagsClear(0x0001U);

                        // 1. Limpieza de caché
                        SCB_CleanInvalidateDCache();

                        // 2. Abortar estados internos del HAL
                        HAL_UART_Abort(&huart3);
                        HAL_UART_AbortTransmit(&huart3);
                        HAL_UART_AbortReceive(&huart3);

                        // 3. Reiniciar DMA TX
                        if (huart3.hdmatx != NULL) {
                            HAL_DMA_Abort(huart3.hdmatx);
                        }

                        // 4. Reiniciar DMA RX
                        if (huart3.hdmarx != NULL) {
                            HAL_DMA_Abort(huart3.hdmarx);
                        }

                        // 5. Limpiar FIFO interna del UART
                        __HAL_UART_FLUSH_DRREGISTER(&huart3);

                        // 6. Reconfigurar UART completamente
                        MX_USART3_UART_Init();

                        // 7. Rehabilitar interrupciones
                        __HAL_UART_ENABLE_IT(&huart3, UART_IT_TC);
                        __HAL_UART_ENABLE_IT(&huart3, UART_IT_IDLE);
                        __HAL_UART_ENABLE_IT(&huart3, UART_IT_ERR);

                        // 8. Rehabilitar periférico
                        __HAL_UART_ENABLE(&huart3);
                        __HAL_UART_ENABLE(&huart3);

                        // 9. Pequeño retardo de estabilización
                        for (volatile int i = 0; i < 1000; i++);

                        osThreadFlagsClear(0x0001U);
                        break;


                    default:

                        osThreadFlagsClear(0x0001U);
                        HAL_UART_AbortTransmit(&huart3);
                        osThreadFlagsClear(0x0001U);
                        break;
                }

                vPortFree(msgPtr);
                msgPtr = NULL;

                vPortFree(msg);
                msg = NULL;
            }
        }
    }
}
