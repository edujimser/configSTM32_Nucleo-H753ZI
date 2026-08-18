#ifndef TESTGUART_H
#define TESTGUART_H

#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include "queue.h"
#include "task.h"
#include <stdbool.h>
#include <stdio.h>

typedef enum {
	FAIL_NULL = 0,
    ESTADO_PARADA,
    ESTADO_CORRIENDO,
} E_Status_TestUart;

// ==========================================================================================================================================
// INSTANCIA
// ==========================================================================================================================================
typedef struct TestUartInstance* I_TestUart;
I_TestUart TestUart_Init(bool Activacion);

// ==========================================================================================================================================
// FUNCIONES INSTANCIA
// ==========================================================================================================================================
//GET -->
E_Status_TestUart TestUart_GetStatus(I_TestUart TestUart);
bool TestUart_GetActivacion(I_TestUart TestUart);
//SET -->
void TestUart_SetStatus(I_TestUart TestUart, E_Status_TestUart status);
void TestUart_SetActivacion(I_TestUart TestUart, bool activacion);




void Prueba_TX(I_TestUart TestUart, bool Activacion);
void Prueba_Longitud_Maxima(void);
void Prueba_Mensajes_Vacios(void);
void Prueba_Caracteres_Invalidos(void);
void Prueba_Saturacion_Cola(void);
void PruebaConcurrente_Tiempo(uint32_t tiempo_ms);
void Prueba_Abortos_DMA_Repetidos(void);
void Prueba_FIFO_Corrupta(void);
void Prueba_Puntero_Reutilizado(void);


#endif /* TESTGUART_H */
