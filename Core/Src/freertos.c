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
#include <stdio.h>
#include "queue.h"
#include "LedDataVision/LedData.h"
#include "usart.h"
#include "stm32h7xx_hal_uart.h"
#include "MsgUart/TestUart/TestUart.h"
#include "MsgUart/RxUart/RxUart.h"
#include "MsgUart/TxUart/TxUart.h"
#include "SPI_Flash_Memory/SPI_Flash_Memory.h"
#include "Network/network.h"
#include "Network/PageWeb/modulo2_threads.h"
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
#define QUEUE_UART_RX_SIZE 10U
#define QUEUE_UART_TX_SIZE 15U

I_TestUart Uart1;
extern SPI_HandleTypeDef hspi1;
uint8_t flash_id[3] = {0, 0, 0};

/* ==============================================================================
 * 1. Definición de Atributos de Tareas (Thread Attributes)
 * ============================================================================== */
#define STACK_SIZE_INIT_TASK 512 * 4
const osThreadAttr_t ThreadAttr_InitTask = {
  .name       = "Task_Init",
  .stack_size = STACK_SIZE_INIT_TASK,
  .priority   = (osPriority_t) osPriorityHigh,
};

#define STACK_SIZE_STATUS_SYSTEM 256 * 4
const osThreadAttr_t ThreadAttr_StatusSystem = {
  .name       = "Task_StatusSystem",
  .stack_size = STACK_SIZE_STATUS_SYSTEM,
  .priority   = (osPriority_t) osPriorityLow,
};

#define STACK_SIZE_UART_TX 256 * 8
const osThreadAttr_t ThreadAttr_Uart_Tx = {
  .name       = "Task_UartTx",
  .stack_size = STACK_SIZE_UART_TX,
  .priority   = (osPriority_t) osPriorityBelowNormal,
};

#define STACK_SIZE_UART_RX 256 * 12
const osThreadAttr_t ThreadAttr_Uart_Rx = {
  .name       = "Task_UartRx",
  .stack_size = STACK_SIZE_UART_RX,
  .priority   = (osPriority_t) osPriorityBelowNormal,
};

#define STACK_SIZE_NETWORK 2048 * 10
static const osThreadAttr_t ThreadAttr_Network = {
  .name       = "Task_Network",
  .stack_size = STACK_SIZE_NETWORK,
  .priority   = (osPriority_t) osPriorityLow1,
};


/* ==============================================================================
 * 2. Definición de Identificadores de Tareas (Thread Handles)
 * ============================================================================== */

osThreadId_t ThreadId_InitTask;
osThreadId_t ThreadId_StatusSystem;
osThreadId_t ThreadId_Uart_Tx;
osThreadId_t ThreadId_Uart_Rx;
osThreadId_t ThreadId_Network;


/* USER CODE END Variables */
/* Definitions for defaultTask */
osThreadId_t defaultTaskHandle;
const osThreadAttr_t defaultTask_attributes = {
  .name = "defaultTask",
  .stack_size = 512 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */
void ThreadFunc_InitTask(void *argument);

/* USER CODE END FunctionPrototypes */

void StartDefaultTask(void *argument);

void MX_FREERTOS_Init(void); /* (MISRA C 2004 rule 8.1) */

/* Hook prototypes */
void configureTimerForRunTimeStats(void);
unsigned long getRunTimeCounterValue(void);
void vApplicationMallocFailedHook(void);

/* USER CODE BEGIN 1 */
/* Functions needed when configGENERATE_RUN_TIME_STATS is on */
__weak void configureTimerForRunTimeStats(void)
{
  Modulo2_RunTimeInit();
}

__weak unsigned long getRunTimeCounterValue(void)
{
  return Modulo2_RunTimeGet();
}
/* USER CODE END 1 */

/* USER CODE BEGIN 5 */
void vApplicationMallocFailedHook(void)
{
   /* vApplicationMallocFailedHook() will only be called if
   configUSE_MALLOC_FAILED_HOOK is set to 1 in FreeRTOSConfig.h. It is a hook
   function that will get called if a call to pvPortMalloc() fails.
   pvPortMalloc() is called internally by the kernel whenever a task, queue,
   timer or semaphore is created. It is also called by various parts of the
   demo application. If heap_1.c or heap_2.c are used, then the size of the
   heap available to pvPortMalloc() is defined by configTOTAL_HEAP_SIZE in
   FreeRTOSConfig.h, and the xPortGetFreeHeapSize() API function can be used
   to query the size of free heap space that remains (although it does not
   provide information on how the remaining heap might be fragmented). */
}
/* USER CODE END 5 */

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
	Queue_Uart_Tx = osMessageQueueNew(QUEUE_UART_TX_SIZE, sizeof(S_UartMsg *), NULL);
	Queue_Uart_Rx = osMessageQueueNew(QUEUE_UART_RX_SIZE, sizeof(S_UartMsg *), NULL);
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of defaultTask */
  defaultTaskHandle = osThreadNew(StartDefaultTask, NULL, &defaultTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */

	// 1. Tarea de Inicialización del Sistema
		ThreadId_InitTask = osThreadNew(ThreadFunc_InitTask, NULL, &ThreadAttr_InitTask);

	// 2. Tarea de Estado del Sistema (LEDs/Sensores)
		ThreadId_StatusSystem = osThreadNew(StartGreenYellowRedTask, &startConfiguration, &ThreadAttr_StatusSystem);

	// 3. Tareas UART (Transmisión y Recepción)
		ThreadId_Uart_Tx = osThreadNew(ThreadFunc_Uart_TX, NULL, &ThreadAttr_Uart_Tx);
		ThreadId_Uart_Rx = osThreadNew(ThreadFunc_Uart_Rx, NULL, &ThreadAttr_Uart_Rx);

	// 4. NetWork
		//Archivo propio

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
    osDelay(1000000);
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
  Uart1 = TestUart_Init(false);
  osDelay(100);
  Flash_Init(&spi_flash_memory_1, &hspi1, SPI1_CS_GPIO_Port, SPI1_CS_Pin, true);
  osDelay(300);
  Flash_Load_Init(&spi_flash_memory_1);
  osDelay(300);
  Flash_Monitor_Init(&spi_flash_memory_1);
  osDelay(300);
  ThreadId_Network = osThreadNew(ThreadFunc_Network, NULL, &ThreadAttr_Network);
  osDelay(100);

  osThreadExit();
}



/* USER CODE END Application */

