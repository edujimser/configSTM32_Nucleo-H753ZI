#include <string.h>
#include "main.h"
#include <stdint.h>
#include <stdlib.h>
#include "FreeRTOS.h"
#include "task.h"
#include "queue.h"
#include "usart.h"
#include "stm32h7xx_hal_uart.h"
#include "MsgUart/MsgUart.h"

// ==========================================================================================================================================
// TRANSMISIÓNM
// ==========================================================================================================================================

uint32_t Stack_libre_uarth;
uint32_t Stack_libre_uarth_critic;

osMessageQueueId_t hUartQueue = NULL;
osThreadId_t UartTaskHandle = NULL;

void MSG_Enviar(const char *texto) {

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


// PRUEBAS INDIVIDUALES ======================================================================================================================
bool pruebasEjecutadas = true;

void Prueba_TX()
{

	MSG_Enviar("Hola, esto es una prueba desde una tarea de FreeRTOS!\r\n");
	        vTaskDelay(pdMS_TO_TICKS(500));

	        if (!pruebasEjecutadas)
	        {
	            pruebasEjecutadas = true;

	            MSG_Enviar(">>> INICIANDO BATERÍA DE PRUEBAS DE ROBUSTEZ UART <<<\r\n");
	            vTaskDelay(pdMS_TO_TICKS(5000));

	            // 1) Longitud máxima superada
	            Prueba_Longitud_Maxima();
	            vTaskDelay(pdMS_TO_TICKS(5000));

	            // 2) Mensajes vacíos y NULL
	            Prueba_Mensajes_Vacios();
	            vTaskDelay(pdMS_TO_TICKS(5000));

	            // 3) Caracteres inválidos
	            Prueba_Caracteres_Invalidos();
	            vTaskDelay(pdMS_TO_TICKS(5000));

	            // 4) Saturación de cola
	            Prueba_Saturacion_Cola();
	            vTaskDelay(pdMS_TO_TICKS(5000));

	            // 5) Condición de carrera entre tareas
	            //    (requiere que vTaskPruebaConcurrente esté creada)
	            PruebaConcurrente_Tiempo(20000);
	            vTaskDelay(pdMS_TO_TICKS(5000));

	            // 6) Abortos DMA repetidos
	            Prueba_Abortos_DMA_Repetidos();

	            // 7) FIFO corrupta (tu prueba original)
	            Prueba_FIFO_Corrupta();

	            // 8) Puntero reutilizado
	            Prueba_Puntero_Reutilizado();

	            // 9) Latencia extrema (requiere tareas pesadas creadas)
	            MSG_Enviar(">>> Prueba de latencia extrema (carga alta) <<<\r\n");
	            vTaskDelay(pdMS_TO_TICKS(500));

	            MSG_Enviar(">>> BATERÍA DE PRUEBAS FINALIZADA <<<\r\n");
	        }
}

// =======================
// 1) PRUEBA LONGITUD MÁXIMA
// =======================
void Prueba_Longitud_Maxima(void)
{
    MSG_Enviar(">>> PRUEBA: Mensaje que supera LONGITUD_MAX_UART <<<\r\n");

    char bufferLargo[200];
    memset(bufferLargo, 'A', sizeof(bufferLargo) - 1);
    bufferLargo[sizeof(bufferLargo) - 1] = '\0';

    MSG_Enviar(bufferLargo);   // Debe disparar MSG_Enviar_Error()
    vTaskDelay(pdMS_TO_TICKS(500));
}

// =======================
// 2) PRUEBA MENSAJES VACÍOS / NULL
// =======================
void Prueba_Mensajes_Vacios(void)
{
    MSG_Enviar(">>> PRUEBA: Mensajes vacíos y NULL <<<\r\n");

    MSG_Enviar("");        // Texto vacío
    MSG_Enviar(NULL);      // Puntero nulo

    vTaskDelay(pdMS_TO_TICKS(500));
}

// =======================
// 3) PRUEBA CARACTERES INVÁLIDOS
// =======================
void Prueba_Caracteres_Invalidos(void)
{
    MSG_Enviar(">>> PRUEBA: Caracteres inválidos <<<\r\n");

    char bufferInvalidos[32];
    bufferInvalidos[0] = 0x01; // control no permitido
    bufferInvalidos[1] = 0x02;
    bufferInvalidos[2] = '\r'; // permitido
    bufferInvalidos[3] = '\n'; // permitido
    bufferInvalidos[4] = '\0';

    MSG_Enviar(bufferInvalidos);  // Debe activar MSG_Enviar_Error()
    vTaskDelay(pdMS_TO_TICKS(500));
}

// =======================
// 4) PRUEBA SATURACIÓN DE COLA
// =======================
void Prueba_Saturacion_Cola(void)
{
    MSG_Enviar(">>> PRUEBA: Saturación de cola UART (segura) <<<\r\n");

    // 1. Llenamos la cola con mensajes válidos
    for (int i = 0; i < 200; i++)
    {
        MSG_Enviar("Mensaje para llenar la cola\r\n");
        vTaskDelay(pdMS_TO_TICKS(1));
    }

    // 2. Ahora la cola debería estar llena → MSG_Enviar fallará internamente
    MSG_Enviar("Intentando enviar con cola llena...\r\n");

    // 3. Este mensaje debería activar la ruta de error y liberar memoria
    MSG_Enviar("Mensaje con cola llena\r\n");

    vTaskDelay(pdMS_TO_TICKS(500));

    // 4. Vaciar la cola (la tarea UART lo hará automáticamente)
    MSG_Enviar("Cola debería vaciarse y sistema recuperarse\r\n");

    vTaskDelay(pdMS_TO_TICKS(500));

    MSG_Enviar(">>> FIN PRUEBA: Saturación de cola UART <<<\r\n");
}

// =======================
// 5) PRUEBA CONCURRENCIA ENTRE TAREAS
// =======================
// Tarea secundaria que envía mensajes muy rápido
void PruebaConcurrente_Tiempo(uint32_t tiempo_ms)
{
    uint32_t inicio = HAL_GetTick();   // Tiempo actual en ms

    while ((HAL_GetTick() - inicio) < tiempo_ms)
    {
        MSG_Enviar("Tarea concurrente: mensaje rápido\r\n");

        // Pequeña pausa para no saturar el sistema
        HAL_Delay(10);
    }

    MSG_Enviar("Prueba concurrente finalizada por tiempo\r\n");
}

// =======================
// 6) PRUEBA ABORTOS DMA REPETIDOS
// =======================
void Prueba_Abortos_DMA_Repetidos(void)
{
    MSG_Enviar(">>> PRUEBA: Abortos DMA repetidos <<<\r\n");

    static char buffer[64] = "Mensaje DMA para abortar repetidamente\r\n";

    for (int i = 0; i < 20; i++)
    {
        SCB_CleanDCache_by_Addr((uint32_t*)buffer, sizeof(buffer));

        HAL_UART_Transmit_DMA(&huart3, (uint8_t*)buffer, strlen(buffer));
        vTaskDelay(pdMS_TO_TICKS(1));

        HAL_UART_AbortTransmit(&huart3);
        vTaskDelay(pdMS_TO_TICKS(10));
    }

    MSG_Enviar("Fin de prueba de abortos DMA repetidos\r\n");
    vTaskDelay(pdMS_TO_TICKS(500));
}

// =======================
// 7) PRUEBA FIFO CORRUPTA (TU ORIGINAL)
// =======================
void Prueba_FIFO_Corrupta(void)
{
    MSG_Enviar(">>> PRUEBA: FIFO UART corrupta <<<\r\n");
    vTaskDelay(pdMS_TO_TICKS(500));

    static char bufferPrueba[256];
    strcpy(bufferPrueba, ">>> PRUEBA: Corrompiendo FIFO interna <<<\r\n");

    SCB_CleanDCache_by_Addr((uint32_t*)bufferPrueba, 256);

    HAL_UART_Transmit_DMA(&huart3, (uint8_t*)bufferPrueba, strlen(bufferPrueba));
    vTaskDelay(pdMS_TO_TICKS(5));

    HAL_UART_AbortTransmit(&huart3);
    vTaskDelay(pdMS_TO_TICKS(500));

    char diag[256];
    snprintf(diag, sizeof(diag),
             "ESTADO TRAS CORRUPCIÓN FIFO:\r\n"
             "UART State: %d\r\n"
             "DMA TX State: %d\r\n"
             "UART ISR: 0x%08lX\r\n",
             (int)HAL_UART_GetState(&huart3),
             (huart3.hdmatx ? (int)huart3.hdmatx->State : -1),
             huart3.Instance->ISR);

    MSG_Enviar(diag);
    vTaskDelay(pdMS_TO_TICKS(500));
}

// =======================
// 8) PRUEBA PUNTERO REUTILIZADO
// =======================
void Prueba_Puntero_Reutilizado(void)
{
    MSG_Enviar(">>> PRUEBA: Puntero reutilizado durante DMA <<<\r\n");

    static char buffer[128];
    strcpy(buffer, "Mensaje inicial para DMA\r\n");

    SCB_CleanDCache_by_Addr((uint32_t*)buffer, sizeof(buffer));
    HAL_UART_Transmit_DMA(&huart3, (uint8_t*)buffer, strlen(buffer));

    // Modificamos el buffer mientras DMA podría seguir
    vTaskDelay(pdMS_TO_TICKS(1));
    strcpy(buffer, "Mensaje modificado mientras DMA está en curso\r\n");

    vTaskDelay(pdMS_TO_TICKS(100));
    MSG_Enviar("Fin de prueba de puntero reutilizado\r\n");
    vTaskDelay(pdMS_TO_TICKS(500));
}

// =======================
// 9) PRUEBA KERNEL NO INICIADO
// =======================
// Llamar a esto ANTES de osKernelStart(), por ejemplo en main()
void Prueba_Kernel_No_Iniciado(void)
{
    MSG_Enviar("Mensaje antes de arrancar el kernel\r\n");
    // Debe ser ignorado por MSG_Enviar() (osKernelGetState != osKernelRunning)
}





// ==========================================================================================================================================
// RECEPCION
// ==========================================================================================================================================
osMessageQueueId_t hUartQueueRX = NULL;
osThreadId_t UartRXTaskHandle = NULL;
ALIGN_32BYTES(uint8_t rxDmaBuffer[LONGITUD_MAX_UART_RX]);
ALIGN_32BYTES(uint8_t processingBuffer[LONGITUD_MAX_UART_RX]);
uint16_t receivedDataSize = 0;

void vTaskUartRxHandler (void *argument)
{
    vTaskDelay(pdMS_TO_TICKS(100));

    if (huart3.hdmarx != NULL) {
        HAL_UARTEx_ReceiveToIdle_DMA(&huart3, rxDmaBuffer, LONGITUD_MAX_UART_RX);
        __HAL_DMA_DISABLE_IT(huart3.hdmarx, DMA_IT_HT);
    }

    for (;;) {
        // Esperamos a que la ISR nos notifique que hay datos nuevos (bloqueo eficiente indefinido)
        uint32_t notifiedValue = ulTaskNotifyTake(pdTRUE, portMAX_DELAY);

        if (notifiedValue > 0 && receivedDataSize > 0) {
            // Ya tenemos los datos seguros en 'processingBuffer' y su tamaño en 'receivedDataSize'

            if (receivedDataSize >= 4) {
                if (strncmp((char *)processingBuffer, "PRUEBA TX", 9) == 0) {
                	pruebasEjecutadas = false;
                	Prueba_TX();
                }
                else {
                    MSG_Enviar("Comando no reconocido\r\n");
                }
            }

            // Limpiamos por seguridad
            receivedDataSize = 0;
        }
    }
}
