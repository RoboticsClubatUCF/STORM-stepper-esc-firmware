/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * File Name          : app_freertos.c
  * Description        : Code for freertos applications
  ******************************************************************************
  * @attention
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
#include "stm32g4xx_hal_spi.h"
#include "task.h"
#include "main.h"
#include <stdint.h>
#include <stdlib.h>
#include "app_freertos.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/**
 * @brief RTOS Task for Trinamic Stepper motor driver. This task will communicate with the driver via SPI1.
 * 
 * @param pvParameters 
 */
void Trinamic_SPI_Task(void *pvParameters) {
  uint8_t *RX_Buffer = calloc(1000, sizeof(int));

  for (;;) {
    SerialSPIParams_t *parms = (*SerialSPIParams_t)pvParmeters;

    HAL_SPI_Receive_DMA(*parms->hspi1, RX_Buffer, 1000);
  }

  free(RX_Buffer);

  /* As per the first code listing above. */
  vTaskDelete(NULL);
}

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

/* USER CODE END Variables */

/* Private function prototypes -----------------------------------------------*/
/* USER CODE BEGIN FunctionPrototypes */

/* USER CODE END FunctionPrototypes */

/* Private application code --------------------------------------------------*/
/* USER CODE BEGIN Application */

/* USER CODE END Application */

