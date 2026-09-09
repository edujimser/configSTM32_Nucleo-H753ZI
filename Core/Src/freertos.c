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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <string.h>
#include "queue.h"
#include "LedDataVision/LedData.h"
#include "usart.h"
#include "stm32h7xx_hal_uart.h"
#include "MsgUart/TestUart/TestUart.h"
#include "MsgUart/RxUart/RxUart.h"
#include "MsgUart/TxUart/TxUart.h"
#include "SPI_Flash_Memory/SPI_Flash_Memory.h"
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
I_TestUart TestUart_1;
extern SPI_HandleTypeDef hspi1;
uint8_t flash_id[3] = {0, 0, 0};

/* ==============================================================================
 * 1. Definición de Atributos de Tareas (Thread Attributes)
 * ============================================================================== */

const osThreadAttr_t ThreadAttr_InitTask = {
  .name       = "Task_Init",
  .stack_size = 512 * 4,
  .priority   = (osPriority_t) osPriorityHigh,
};

const osThreadAttr_t ThreadAttr_Test = {
  .name       = "Task_Test",
  .stack_size = 256 * 12,
  .priority   = (osPriority_t) osPriorityLow1,
};

const osThreadAttr_t ThreadAttr_StatusSystem = {
  .name       = "Task_StatusSystem",
  .stack_size = 256 * 4,
  .priority   = (osPriority_t) osPriorityLow,
};

const osThreadAttr_t ThreadAttr_Uart_Tx = {
  .name       = "Task_UartTx",
  .stack_size = 256 * 8,
  .priority   = (osPriority_t) osPriorityBelowNormal,
};

const osThreadAttr_t ThreadAttr_Uart_Rx = {
  .name       = "Task_UartRx",
  .stack_size = 256 * 12,
  .priority   = (osPriority_t) osPriorityBelowNormal,
};

/* ==============================================================================
 * 2. Definición de Identificadores de Tareas (Thread Handles)
 * ============================================================================== */

osThreadId_t ThreadId_InitTask;
osThreadId_t ThreadId_Test;
osThreadId_t ThreadId_StatusSystem;
osThreadId_t ThreadId_Uart_Tx;
osThreadId_t ThreadId_Uart_Rx;

/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 128 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void ThreadFunc_InitTask(void *argument);
void ThreadFunc_Test(void *argument);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

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
	Queue_Uart_Tx = osMessageQueueNew(15, sizeof(S_UartMsg *), NULL);
	Queue_Uart_Rx = osMessageQueueNew(10, sizeof(S_UartMsg *), NULL);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */

	// 1. Tarea de Inicialización del Sistema
	ThreadId_InitTask = osThreadNew(ThreadFunc_InitTask, NULL, &ThreadAttr_InitTask);

	// 2. Tarea de Pruebas (SPI Flash)
	ThreadId_Test = osThreadNew(ThreadFunc_Test, NULL, &ThreadAttr_Test);

	// 3. Tarea de Estado del Sistema (LEDs/Sensores)
	ThreadId_StatusSystem = osThreadNew(StartGreenYellowRedTask, &startConfiguration, &ThreadAttr_StatusSystem);

	// 4. Tareas UART (Transmisión y Recepción)
	ThreadId_Uart_Tx = osThreadNew(ThreadFunc_Uart_TX, NULL, &ThreadAttr_Uart_Tx);
	ThreadId_Uart_Rx = osThreadNew(ThreadFunc_Uart_Rx, NULL, &ThreadAttr_Uart_Rx);

  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

}

/* USER CODE BEGIN Header_StartDefaultTask */
/* USER CODE END Header_StartDefaultTask */
void StartDefaultTask(void *argument)
{
  /* USER CODE BEGIN StartDefaultTask */
  /* Infinite loop */
  for(;;)
  {
    osDelay(5000);
  }
  /* USER CODE END StartDefaultTask */
}

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* ==============================================================================
 * Implementación de las Funciones de Tarea (Thread Functions)
 * ============================================================================== */

/**
 * @brief Tarea encargada de inicializar periféricos o módulos que requieren
 *        el RTOS activo (ej. uso de retardos de OS, colas, etc.).
 *        Una vez finaliza, se auto-destruye.
 */
void ThreadFunc_InitTask(void *argument)
{
  TestUart_1 = TestUart_Init(false);
  osDelay(10);

  Flash_Init(&spi_flash_memory_1, &hspi1, SPI1_CS_GPIO_Port, SPI1_CS_Pin, true);

  osDelay(10);

  osThreadExit();
}


/**
 * @brief Tarea de pruebas continuas. En este caso, verifica la comunicación
 *        SPI con la memoria Flash periódicamente.
 */
void ThreadFunc_Test(void *argument)
{
    for (;;)
    {
    	osDelay(5000);

    	Flash_Read_ID(&spi_flash_memory_1);

        osDelay(5000);
    }
}

/* USER CODE END Application */

