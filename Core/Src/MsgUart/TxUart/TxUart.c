#include "MsgUart/TxUart/TxUart.h"
#include <string.h>
#include "main.h"
#include <stdint.h>
#include <stdlib.h>
#include "FreeRTOS.h"

#include "task.h"
#include "queue.h"
#include "usart.h"
#include "MsgUart/TestUart/TestUart.h"


#define FLAG_TX_DONE   (1U << 0)   // lo activa HAL_UART_TxCpltCallback
#define FLAG_TX_ERR    (1U << 1)   // lo activa HAL_UART_ErrorCallback

// ==========================================================================================
// CONFIGURACION TYPE MSG
// ==========================================================================================
static const char MSG_TYPETEXT[TYPE_MSG_UART_COUNT][LONG_TYPE_COMAND] = {
    [MSG_GENERAL] = "[GEN]",
    [MSG_FLASH_1] = "[FL1]",
    [MSG_WEB]     = "[WEB]",
    [MSG_ERROR]   = "[ERR]",
};
// ==========================================================================================
// TRANSMISIÓNM
// ==========================================================================================

uint32_t Stack_libre_uarth;
volatile S_StatusUartMsg StatusUartMsg = {0};
extern I_TestUart Uart1;
osMessageQueueId_t Queue_Uart_Tx = NULL;


void MSG_Enviar(E_TYPE_MSG_UART typeMSG, const char *texto, I_TestUart TestUart) {

	if(TestUart_GetStatus(TestUart) == ESTADO_CORRIENDO ||
	   osKernelGetState() != osKernelRunning		    ||
	   Queue_Uart_Tx == NULL                            ||
	   texto == NULL                                    ||
	   uxQueueSpacesAvailable(Queue_Uart_Tx) == 0        ) return;


    if (MSG_Enviar_Longitud_Limit(texto)) {
    	MSG_Enviar(MSG_ERROR,"Se ha superado el limite maximo de caracteres \r\n", Uart1);
        return;
    }

    if (MSG_Enviar_Caract_Valid(texto)) {
    	MSG_Enviar(MSG_ERROR,"Se ha introducido un carcacter fuera de rango \r\n", Uart1);
        return;
    }

    //PREFIJO + TEXT -->
    const char *prefijo;
    char textBuf[LONG_TYPE_TEXT];

    if(typeMSG >= TYPE_MSG_UART_COUNT){
    	prefijo = "???";
    }else{
    	prefijo = MSG_TYPETEXT[typeMSG];
    }

    snprintf(textBuf,sizeof(textBuf),"%s %s", prefijo,texto);

    if (MSG_Enviar_Longitud_LimitBuf(textBuf)) {
    	MSG_Enviar(MSG_ERROR,"Se ha superado el limite maximo de caracteres \r\n");
	   return;
   }

    //RESERVE MEMORY HEAP
    S_UartMsg *msg = pvPortMalloc(sizeof(S_UartMsg));
    if (msg == NULL) return;


    msg->len = strlen(textBuf) + 1;


    msg->Ptr = pvPortMalloc(msg->len);
    if (msg->Ptr == NULL) {

        vPortFree(msg);
        return;
    }

    strncpy(msg->Ptr, textBuf, msg->len - 1);

    // Aseguramos que el último byte sea '\0'.
    msg->Ptr[msg->len - 1] = '\0';


    if (osMessageQueuePut(Queue_Uart_Tx, &msg, 0, UART_QUEUE_TIMEOUT_TICKS) != osOK) {

        vPortFree(msg->Ptr);
        vPortFree(msg);
        StatusUartMsg.Queue.Msg_Queue_discarded ++;
    }
}

void MSG_Enviar_Test(const char *texto) {

    // 1. Comprobación: el kernel debe estar en ejecución.
    //    Si FreeRTOS aún no ha arrancado, no podemos usar colas.
    if (osKernelGetState() != osKernelRunning) return;

    // 2. Comprobación: la cola debe existir.
    //    Si la cola no está creada, no enviamos nada.
    if (Queue_Uart_Tx == NULL) return;

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
    if (osMessageQueuePut(Queue_Uart_Tx, &msg, 0, UART_QUEUE_TIMEOUT_TICKS) != osOK) {

        // Si la cola está llena o falla el envío:
        // liberamos el texto y la estructura para evitar fugas de memoria.
        vPortFree(msg->Ptr);
        vPortFree(msg);

        //Status Uart Queue full
        StatusUartMsg.Queue.Msg_Queue_discarded;
    }
}



char MSG_Enviar_Longitud_Limit(const char *texto){

	size_t len = strlen(texto) + 1;

	if(len > LONG_TYPE_TEXT){
		return true;
	}
	 return false;
}

char MSG_Enviar_Longitud_LimitBuf(const char *texto){

	size_t len = strlen(texto) + 1;

	if(len > LONG_MAX_UART_TX){
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


// ==========================================================================================
// HILO FUCTIONS
// =========================================================================================
void ThreadFunc_Uart_TX(void *argument) {
	(void)argument;
    S_UartMsg *msg = {0};



    MSG_UpdateStatusStatic();


    for (;;) {

    	MSG_UpdateStatusDinamic();

    	//NO MSGQUEUE -->
    	if (osMessageQueueGet(Queue_Uart_Tx, &msg, NULL, osWaitForever) != osOK) {
			continue;
		}

    	//YES MSGQUEUE
		if(msg == NULL ){
			msg = NULL;
			StatusUartMsg.Queue.Msg_Queue_Invalid ++ ;
			continue;
		}

		if(msg->Ptr == NULL || msg->len == NULL){
			vPortFree(msg->Ptr);
			vPortFree(msg);
			msg = NULL;
			StatusUartMsg.Queue.Msg_Queue_Invalid ++ ;
			continue;
		}


		if(msg->len == 0 || msg->len > LONG_MAX_UART_TX){
			vPortFree(msg->Ptr);
			vPortFree(msg);
			msg = NULL;
			StatusUartMsg.Queue.Msg_Queue_NoLen ++ ;
			continue;
		}


        //MSG VALID -->
        char     *msgPtr = msg->Ptr;
        uint16_t  len    = (uint16_t)msg->len;
        bool      send   = false;              // se reinicia en CADA mensaje

        //CACHE: el DMA lee de RAM -->
        SCB_CleanDCache_by_Addr((uint32_t *)msgPtr, (int32_t)len);

        //ESTADO HUART -->
        HAL_UART_StateTypeDef st = huart3.gState;
        MSG_UpdateStatusState(st);


        switch (st) {

            case HAL_UART_STATE_READY:break;
            case HAL_UART_STATE_BUSY:
            case HAL_UART_STATE_BUSY_TX:
                HAL_UART_AbortTransmit(&huart3);
                break;

            case HAL_UART_STATE_RESET:
            case HAL_UART_STATE_ERROR:
            default:
            {
                StatusUartMsg.TX.Msg_TX_Recovery++;
                StatusUartMsg.TX.Msg_TX_Recovery_LastTick = osKernelGetTickCount();
                bool rec_ok = true;


                if (HAL_UART_Abort(&huart3) != HAL_OK) {
                    StatusUartMsg.TX.Msg_TX_Recovery_AbortFail++;
                }


                if (HAL_UART_DeInit(&huart3) != HAL_OK) {
                    rec_ok = false;
                }


                osDelay(1);


                if (rec_ok && HAL_UART_Init(&huart3) != HAL_OK) {
                    rec_ok = false;
                }

				#if defined(USART_CR1_FIFOEN)
					if (rec_ok &&
					   (HAL_UARTEx_SetTxFifoThreshold(&huart3, UART_TXFIFO_THRESHOLD_1_8) != HAL_OK ||
						HAL_UARTEx_SetRxFifoThreshold(&huart3, UART_RXFIFO_THRESHOLD_1_8) != HAL_OK ||
						HAL_UARTEx_DisableFifoMode(&huart3)                               != HAL_OK)) {
						rec_ok = false;
					}
				#endif

                if (rec_ok) {

                    __HAL_UART_CLEAR_PEFLAG(&huart3);
                    __HAL_UART_CLEAR_FEFLAG(&huart3);
                    __HAL_UART_CLEAR_NEFLAG(&huart3);
                    __HAL_UART_CLEAR_OREFLAG(&huart3);
                    __HAL_UART_CLEAR_IDLEFLAG(&huart3);
                    __HAL_UART_FLUSH_DRREGISTER(&huart3);


                    if (huart3.gState != HAL_UART_STATE_READY ||
                        huart3.RxState != HAL_UART_STATE_READY) {
                        rec_ok = false;
                    }
                }

                if (rec_ok) {

                    StatusUartMsg.TX.Msg_TX_Recovery_FailsContinues = 0;
                }
                else {

                    StatusUartMsg.TX.Msg_TX_Recovery++;
                    uint32_t n = ++StatusUartMsg.TX.Msg_TX_Recovery_FailsContinues;
                    uint32_t espera = (n < 7U) ? (10U << (n - 1U)) : 1000U;
                    osDelay(espera);
                }

                // 10. Flags que la ISR pudo dejar durante el reinicio
                osThreadFlagsClear(FLAG_TX_DONE | FLAG_TX_ERR);
                break;
            }
        }


        if (huart3.gState == HAL_UART_STATE_READY) {

            osThreadFlagsClear(FLAG_TX_DONE | FLAG_TX_ERR);
            uint32_t t0 = osKernelGetTickCount();

            HAL_StatusTypeDef res = HAL_UART_Transmit_DMA(&huart3, (uint8_t *)msgPtr, len);

            if (res == HAL_OK) {

                uint32_t timeout = (((uint32_t)len * 10U * 1000U) / huart3.Init.BaudRate) + UART_TX_MARGEN_MS;
                uint32_t flags   = osThreadFlagsWait(FLAG_TX_DONE | FLAG_TX_ERR, osFlagsWaitAny, timeout);
                uint32_t dt      = osKernelGetTickCount() - t0;

                if (flags & osFlagsError) {                         // TIMEOUT
                    StatusUartMsg.TX.Msg_TX_Timeout++;
                    HAL_UART_AbortTransmit(&huart3);
                }
                else if (flags & FLAG_TX_ERR) {
                    StatusUartMsg.TX.Msg_TX_Error++;
                    if (huart3.gState != HAL_UART_STATE_READY) {
                        HAL_UART_AbortTransmit(&huart3);
                    }
                }
                else {
                    StatusUartMsg.TX.Msg_TX_Ok++;
                    StatusUartMsg.TX.Msg_TX_LenBytes += len;
                    StatusUartMsg.TX.Msg_TX_LastTime  = dt;
                    if (dt > StatusUartMsg.TX.Msg_TX_TimeMax) {
                        StatusUartMsg.TX.Msg_TX_TimeMax = dt;
                    }
                    send = true;
                }
            }
            else if (res == HAL_BUSY) {
                StatusUartMsg.TX.Msg_TX_Busy++;
            }
            else {
                StatusUartMsg.TX.Msg_TX_Error++;
            }
        }


        if (!send) {
            StatusUartMsg.TX.Msg_TX_Discarded++;
        }


        vPortFree(msgPtr);
        msgPtr = NULL;
        vPortFree(msg);
        msg = NULL;
    }
}



static void MSG_UpdateStatusDinamic(void){
	uint32_t now   = osKernelGetTickCount();
	uint32_t free  = osThreadGetStackSpace(osThreadGetId());
	uint32_t use   = osMessageQueueGetCount(Queue_Uart_Tx);
	uint32_t space = osMessageQueueGetSpace(Queue_Uart_Tx);

	//STACK -->
	StatusUartMsg.Thread.Thread_Stack_Uart_TX_Free = free;
	StatusUartMsg.Thread.Thread_Stack_Uart_TX_Use  = StatusUartMsg.Thread.Thread_Stack_Uart_TX - free;

	//VIDA -->
	StatusUartMsg.Thread.Thread_Heartbeat_Uart_TX++;
	StatusUartMsg.Thread.Thread_Ticks_Last   = now;
	StatusUartMsg.Thread.Thread_Ticks_Uptime = now - StatusUartMsg.Thread.Thread_Ticks_T0;

	//QUEUE -->
	StatusUartMsg.Queue.Msg_Queue_Use  = use;
	StatusUartMsg.Queue.Msg_Queue_Free = space;
	if (use > StatusUartMsg.Queue.Msg_Queue_Use_Max) {
		StatusUartMsg.Queue.Msg_Queue_Use_Max = use;
	}
}

static void MSG_UpdateStatusStatic(void){

	//STACK -->
	StatusUartMsg.Thread.Thread_ID            = osThreadGetId();
	StatusUartMsg.Thread.Thread_Stack_Uart_TX = UART_STACK_SIZE_UART_TX;
	StatusUartMsg.Thread.Thread_Ticks_T0      = osKernelGetTickCount();
	//QUEUE
	StatusUartMsg.Queue.Msg_Queue             = UART_QUEUE_UART_TX_SIZE;

}

static void MSG_UpdateStatusState(HAL_UART_StateTypeDef st)
{
    const char *data;


    switch (st) {
        case HAL_UART_STATE_READY:
        	StatusUartMsg.State.Msg_GetState_Count_Ready++;
            data = "STATE READY";
            break;

        case HAL_UART_STATE_BUSY:
        case HAL_UART_STATE_BUSY_TX:
        case HAL_UART_STATE_BUSY_RX:
        case HAL_UART_STATE_BUSY_TX_RX:
        	StatusUartMsg.State.Msg_GetState_Count_Busy++;
            data = "STATE BUSY";
            break;

        case HAL_UART_STATE_TIMEOUT:
        	StatusUartMsg.State.Msg_GetState_Count_Timeout++;
            data = "STATE TIMEOUT";
            break;

        case HAL_UART_STATE_ERROR:
        	StatusUartMsg.State.Msg_GetState_Count_Error++;
            data = "STATE ERROR";
            break;

        case HAL_UART_STATE_RESET:
        default:
        	StatusUartMsg.State.Msg_GetState_Count_Error++;
            data = "STATE UNKNOWN";
            break;
    }

    /* Historial circular */
    uint16_t pos = StatusUartMsg.State.Msg_GetSate_Count_List_Position;

    if (pos >= UART_STATE_LIST_HISTORY) {
        pos = 0;
    }

    StatusUartMsg.State.Msg_GetState_List_history[pos] = data;
    StatusUartMsg.State.Msg_GetSate_Count_List_Position = pos + 1;
}
