/* USER CODE BEGIN Header */
/**
  * @file           : main.c
  * @brief          : Código Nucleo-H753 - 400MHz HSE con 3 Tareas FreeRTOS
  */
/* USER CODE END Header */



/* Includes ---------------------------------------------------------------------------------------------------------------------------------------------------------*/
#include "main.h"      /* Configuración base: Definiciones de pines (Labels) y prototipos de inicialización. */
#include "FreeRTOS.h"  /* Kernel de FreeRTOS: Define las estructuras de datos fundamentales del sistema operativo. */
#include "cmsis_os2.h" /* API CMSIS-RTOS v2: Capa de abstracción para gestionar tareas, semáforos y colas. */
#include "rtc.h"       /* Real Time Clock: Manejo del calendario, hora y alarmas internas del microcontrolador. */
#include "usart.h"     /* USART/UART: Configuración de la comunicación serie (usada para nuestro printf). */
#include "gpio.h"      /* GPIO: Configuración de los puertos de entrada y salida general (LEDs, botones, etc). */


/* Private includes -----------------------------------------------------------------------------------------------------------------------------------------------------*/
/* @brief External library dependencies and system headers */
/* USER CODE BEGIN Includes */

/* AQUÍ: Referencias a archivos de cabecera externos (.h).
   Se incluyen librerías estándar de C o drivers externos necesarios para el proyecto. */
#include <stdio.h>    /* Standard I/O: Para usar printf() y redirección de consola */
#include "LedDataVision/LedData.h" /* Gestión de LEDs: Funciones y estructuras para controlar los LEDs de forma flexible. */


/* USER CODE END Includes */







/* Private typedef  -----------------------------------------------------------------------------------------------------------------------------------------------------*/
/* @brief User-defined data types (Structures, Unions, Enumerations) */
/* USER CODE BEGIN PTD */

/* AQUÍ: Definiciones de nuevos tipos de datos creados por el usuario. Se utiliza para declarar estructuras, uniones o tipos enumerados que organicen la información. */

/* USER CODE END PTD */







/* Private define ------------------------------------------------------------------------------------------------------------------------------------------------------*/
/* @brief Compile-time constants and literal definitions */
/* USER CODE BEGIN PD */

/* AQUÍ: Constantes simbólicas procesadas antes de la compilación. Se utiliza para asignar nombres a valores fijos, mejorando la legibilidad y facilitando cambios globales. */

/* USER CODE END PD */







/* Private macro  ------------------------------------------------------------------------------------------------------------------------------------------------------*/
/* @brief Preprocessor macros for inline logic and bit manipulation */
/* USER CODE BEGIN PM */

/* AQUÍ: Macros de preprocesador que ejecutan lógica en línea.
   Se utiliza para operaciones matemáticas rápidas o manipulación directa de registros y bits. */

/* USER CODE END PM */








/* Private variables ----------------------------------------------------------------------------------------------------------------------------------------------------*/
/* @brief Global scoped variables with internal linkage (static context) */
/* USER CODE BEGIN PV */


/* AQUÍ: Declaración de variables globales con alcance en todo este archivo.
   Se utiliza para almacenar estados, contadores y buffers que deben persistir durante la ejecución. */

/* USER CODE END PV */







/* Private variables ----------------------------------------------------------------------------------------------------------------------------------------------------*/
/* @brief Global scoped variables and internal state buffers (Internal Linkage) */
/* AQUÍ: Declaración de variables globales, handles de SO y estructuras de configuración que deben persistir en RAM. */
/* USER CODE BEGIN PV */
osThreadId_t StatusTaskHandle; 


/* USER CODE END PV */








/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
void MX_FREERTOS_Init(void);


/* Private user code ---------------------------------------------------------*/


/* ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ---           L.1                                                          USER CODE BEGIN 0                                                                              --- */
/* ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */


/* ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ---           L.1                                                          MAIN ENTRY POINT                                                                               --- */
/* ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- */

int main(void)
{
	/* ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- */
	/* ---           L.2                                                          USER CODE BEGIN 1                                                                              --- */
	/* ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- */

	SCB_EnableICache();
	SCB_EnableDCache();
	HAL_Init();
	SystemClock_Config();
  Error_Handler_Init();
	MX_GPIO_Init();
	MX_RTC_Init();
	MX_USART3_UART_Init();
	setvbuf(stdout, NULL, _IONBF, 0);

	/* ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- */
	/* ---           L.2                                                          USER CODE BEGIN 2                                                                              --- */
	/* ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- */
	/* USER CODE BEGIN 2 */
	printf("\r\n==================================\r\n");
	printf("  NUCLEO-H753ZI INICIALIZADA\r\n");
	printf("  Consola UART activa a 115200\r\n");
	printf("==================================\r\n");
	HAL_Delay(100);


	StatusTaskHandle = osThreadNew(StartGreenYellowRedTask, &startConfiguration, &StartGreenYellowRedTask_attributes);



	/* USER CODE END 2 */

	/* Init scheduler */
	osKernelInitialize();  /* Call init function for freertos objects (in cmsis_os2.c) */
	MX_FREERTOS_Init();

	/* Start scheduler */
	osKernelStart();

	/* We should never get here as control is now taken by the scheduler */

	/* Infinite loop */

	/* ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- */
	/* ---           L.2                                                          USER CODE BEGIN 3                                                                              --- */
	/* ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- */
	while (1)
	{

	}

}


/* ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- */
/* ---           L.2                                                          USER CODE BEGIN 4                                                                              --- */
/* ----------------------------------------------------------------------------------------------------------------------------------------------------------------------------- */


/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};
  RCC_CRSInitTypeDef RCC_CRSInitStruct = {0};

  /** Supply configuration update enable
  */
  HAL_PWREx_ConfigSupply(PWR_LDO_SUPPLY);

  /** Configure the main internal regulator output voltage
  */
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  while(!__HAL_PWR_GET_FLAG(PWR_FLAG_VOSRDY)) {}

  /** Configure LSE Drive Capability
  */
  HAL_PWR_EnableBkUpAccess();
  __HAL_RCC_LSEDRIVE_CONFIG(RCC_LSEDRIVE_MEDIUMLOW);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI48|RCC_OSCILLATORTYPE_HSE
                              |RCC_OSCILLATORTYPE_LSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.LSEState = RCC_LSE_ON;
  RCC_OscInitStruct.HSI48State = RCC_HSI48_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 4;
  RCC_OscInitStruct.PLL.PLLN = 400;
  RCC_OscInitStruct.PLL.PLLP = 2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  RCC_OscInitStruct.PLL.PLLR = 2;
  RCC_OscInitStruct.PLL.PLLRGE = RCC_PLL1VCIRANGE_1;
  RCC_OscInitStruct.PLL.PLLVCOSEL = RCC_PLL1VCOWIDE;
  RCC_OscInitStruct.PLL.PLLFRACN = 0;

  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    ErrorHandler.file        = __FILE__;
    ErrorHandler.function    = __func__;
    ErrorHandler.line        = __LINE__;
    ErrorHandler.err_code    = ERR_OSC_PLL;
    ErrorHandler.err_name    = "OSC_CONFIG_FAULT";
    ErrorHandler.description = "Fallo al inicializar el oscilador (HSE/HSI) o el PLL.";
    Error_Handler();
  }
  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2
                              |RCC_CLOCKTYPE_D3PCLK1|RCC_CLOCKTYPE_D1PCLK1;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.SYSCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB3CLKDivider = RCC_APB3_DIV2;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_APB1_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_APB2_DIV2;
  RCC_ClkInitStruct.APB4CLKDivider = RCC_APB4_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    /* Rellenamos la "caja negra" antes de morir */
    ErrorHandler.file        = __FILE__;
    ErrorHandler.function    = __func__;
    ErrorHandler.line        = __LINE__;
    ErrorHandler.err_code    = ERR_FLASH_LATENCY;
    ErrorHandler.err_name    = "CLOCK_CONFIG_FAULT";
    ErrorHandler.description = "Fallo al configurar los buses del sistema o la latencia Flash.";

    Error_Handler();
  }
  /** Enable the SYSCFG APB clock
  */
  __HAL_RCC_CRS_CLK_ENABLE();

  /** Configures CRS
  */
  RCC_CRSInitStruct.Prescaler = RCC_CRS_SYNC_DIV1;
  RCC_CRSInitStruct.Source = RCC_CRS_SYNC_SOURCE_LSE;
  RCC_CRSInitStruct.Polarity = RCC_CRS_SYNC_POLARITY_RISING;
  RCC_CRSInitStruct.ReloadValue = __HAL_RCC_CRS_RELOADVALUE_CALCULATE(48000000,32768);
  RCC_CRSInitStruct.ErrorLimitValue = 34;
  RCC_CRSInitStruct.HSI48CalibrationValue = 32;

  HAL_RCCEx_CRSConfig(&RCC_CRSInitStruct);
}



/* USER CODE END 4 */

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM1 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM1)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

#ifdef USE_FULL_ASSERT
/**
  * @brief  Reports the name of the source file and the source line number
  *         where the assert_param error has occurred.
  * @param  file: pointer to the source file name
  * @param  line: assert_param error line source number
  * @retval None
  */
void assert_failed(uint8_t *file, uint32_t line)
{
  /* USER CODE BEGIN 6 */
  /* User can add his own implementation to report the file name and line number,
     ex: printf("Wrong parameters value: file %s on line %d\r\n", file, line) */
  /* USER CODE END 6 */
}
#endif /* USE_FULL_ASSERT */
