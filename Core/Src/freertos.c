/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention1
  *
  * Copyright (c) 2026 STMicroelectronics.
  * All rights reserved.
  *
  * This software is licensed under terms that can be found in the LICENSE file
  * in the root directory of this software component.
  * If no LICENSE file comes with this software, it is provided AS-IS.
  *
  ******************************************************************************
  */
/* USER CODE END Header */

/* Includes ------------------------------------------------------------------*/
#include "FreeRTOS.h"
#include "task.h"
#include "main.h"
#include "FreeRTOS.h"
#include "cmsis_os2.h"
#include <string.h>


/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "queue.h"
#include "LedDataVision/LedData.h"
#include "MsgUart/MsgUart.h"
#include "usart.h"
#include "stm32h7xx_hal_uart.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
/* USER CODE BEGIN Variables */
// Tarea: TaskPrueba
// Prioridad: osPriorityLow1 (Valor: 9)
osThreadId_t TaskPrueba;
const osThreadAttr_t TaskPuerba_attr = {
  .name = "TaskPrueba",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow1,
};

// Tarea: TaskNormalStatusSystem
// Prioridad: osPriorityLow (Valor: 8)
osThreadId_t TaskNormalStatusSystem;
const osThreadAttr_t StartGreenYellowRedTask_attributes = {
  .name = "TaskNormalStatusSystem",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityLow,
};

// Tarea: UartTask
// Prioridad: osPriorityBelowNormal (Valor: 16)
const osThreadAttr_t UartTask_attr = {
  .name = "TaskUartHandle",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityBelowNormal,
};

// Tarea: UartTask
// Prioridad: osPriorityBelowNormal (Valor: 16)
const osThreadAttr_t UartTask_attr_RX = {
  .name = "TaskUartHandleRX",
  .stack_size = 256 * 2,
  .priority = (osPriority_t) osPriorityBelowNormal,
};
/* USER CODE END Variables */
/* Definitions for defaultTask */


/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void vTaskPrueba(void *argument);
/* USER CODE END FunctionPrototypes */


void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/**
  * @brief  FreeRTOS initialization
  * @param  None
  * @retval None
  */
void MX_FREERTOS_Init(void) {
  /* USER CODE BEGIN Init */
  /* USER CODE END Init */

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
	hUartQueue = osMessageQueueNew(10, sizeof(S_UartMsg *), NULL);
	hUartQueueRX = osMessageQueueNew(10, sizeof(S_UartMsg *), NULL);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
	// 1. Tarea de UartTask (Prioridad: 16 - Below Normal)
	UartTaskHandle = osThreadNew(vTaskUartHandler, NULL, &UartTask_attr);

	// 2. Tarea de Prueba (Prioridad: 9 - Low + 1)
	TaskPrueba = osThreadNew(vTaskPrueba, NULL, &TaskPuerba_attr);

	// 3. Tarea de Estado del Sistema (Prioridad: 8 - Low)
	TaskNormalStatusSystem = osThreadNew(StartGreenYellowRedTask, &startConfiguration, &StartGreenYellowRedTask_attributes);
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/* USER CODE END Header_StartDefaultTask */


/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */
void vTaskPrueba(void *argument)
{
    static bool pruebasEjecutadas = false;

    for (;;)
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
}



/* USER CODE END Application */

