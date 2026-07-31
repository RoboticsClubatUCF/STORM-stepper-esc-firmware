/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.h
  * @brief          : Header for main.c file.
  *                   This file contains the common defines of the application.
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

/* Define to prevent recursive inclusion -------------------------------------*/
#ifndef __MAIN_H
#define __MAIN_H

#include <sys/_intsup.h>
#include <sys/_types.h>
#ifdef __cplusplus
extern "C" {
#endif

/* Includes ------------------------------------------------------------------*/
#include "stm32g4xx_hal.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */

/* USER CODE END Includes */

/* Exported types ------------------------------------------------------------*/
/* USER CODE BEGIN ET */

/* USER CODE END ET */

/* Exported constants --------------------------------------------------------*/
/* USER CODE BEGIN EC */

/* USER CODE END EC */

/* Exported macro ------------------------------------------------------------*/
/* USER CODE BEGIN EM */

/* USER CODE END EM */

/* Exported functions prototypes ---------------------------------------------*/
void Error_Handler(void);

/* USER CODE BEGIN EFP */

typedef enum {
  READ = 0,
  WRITE = 1,
} TRINAMIC_MODE;

// Refer to section 6.1 of datasheet for more info.
typedef enum {
  GCONF_RW = 0x00, // n = 18
  GSTAT_RWC = 0x01, // n = 3
  IFCNT_R = 0x02, // n = 8
  NODECONF_W = 0x03, // n = 8 + 4
  IOIN_R = 0x04, // n = 8 + 8 
  OUTPUT_W = 0x04, // n = 1
  X_COMPARE_W = 0x05, // n = 32
  OTP_PROG_W = 0x06, // n = ?
  OTP_READ_R = 0x07, // n = ?
  FACTORY_CONF_RW = 0x08, // n = 5
  SHORT_CONF_W = 0x09, // n = 19
  DRV_CONF_W = 0x0A, // n = 22
  GLOBAL_SCALER_W = 0x0B, // n = 8
  OFFSET_READ_R = 0x0C, // n = 16
} TRINAMIC_GENERAL_REGISTERS;

// Refer to section 6.5 of datasheet for more info.
typedef enum {
  MSLUT0_W = 0x60, // n = 32
  MSLUT1_W = 0x61, // n = 32
  MSLUT2_W = 0x62, // n = 32
  MSLUT3_W = 0x63, // n = 32
  MSLUT4_W = 0x64, // n = 32
  MSLUT5_W = 0x65, // n = 32
  MSLUT6_W = 0x66, // n = 32
  MSLUT7_W = 0x67, // n = 32

  MSLUTSEL_W = 0x68, // n = 32
  MSLUTSTART_W = 0x69, // n = 8 + 8
  MSCNT_R = 0x6A, // n = 10
  MSCURACT_R = 0x6B, // n = 18
  CHOPCONF_RW = 0x6C, // n = 32
  COOLCONF_W = 0x6D, // n = 25
  DCCTRL_W = 0x6E, // n = 24
  DRV_STATUS = 0x6F, // n = 32
  PWMCONF_W = 0x70, // n = 32
  PWM_SCALE_R = 0x71, // n = 9 + 8
  PWM_AUTO_R = 0x72, // n = 8 + 8
  LOST_STEPS_R = 0x73, // n = 20
} TRINAMIC_MOTOR_DRIVER_REGISTERS;

typedef struct {
  char status_stop_r;
  char status_stop_i;
  char position_reached;
  char velocity_reached;
  char standstill;
  char sg2;
  char driver_error;
  char reset_flag;
} spi_status;

typedef struct {
  char address: 7;
  SPI_HandleTypeDef *hspi1;
} SerialSPIParams_t;

// Total of 40 bits wide
typedef struct __attribute((packed)) {
  TRINAMIC_MODE mode: 1;
  unsigned char register_address: 7;
  unsigned int data: 32;
} Trinamic_Datagram;

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
