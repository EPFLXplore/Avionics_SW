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

/* USER CODE END EFP */

/* Private defines -----------------------------------------------------------*/
#define ST_MUX_Pin GPIO_PIN_14
#define ST_MUX_GPIO_Port GPIOC
#define PG_VBAT_Pin GPIO_PIN_15
#define PG_VBAT_GPIO_Port GPIOC
#define NRST_Pin GPIO_PIN_10
#define NRST_GPIO_Port GPIOG
#define TIM_LEDS_1_Pin GPIO_PIN_0
#define TIM_LEDS_1_GPIO_Port GPIOA
#define TIM_LEDS_2_Pin GPIO_PIN_1
#define TIM_LEDS_2_GPIO_Port GPIOA
#define HX2_DATA_Pin GPIO_PIN_2
#define HX2_DATA_GPIO_Port GPIOA
#define HX2_CLK_Pin GPIO_PIN_3
#define HX2_CLK_GPIO_Port GPIOA
#define TIM_SERVO_4_Pin GPIO_PIN_11
#define TIM_SERVO_4_GPIO_Port GPIOB
#define TIM_SERVO_3_Pin GPIO_PIN_13
#define TIM_SERVO_3_GPIO_Port GPIOB
#define TIM_SERVO_2_Pin GPIO_PIN_14
#define TIM_SERVO_2_GPIO_Port GPIOB
#define TIM_SERVO_1_Pin GPIO_PIN_15
#define TIM_SERVO_1_GPIO_Port GPIOB
#define VIN_ALERT_Pin GPIO_PIN_15
#define VIN_ALERT_GPIO_Port GPIOA
#define SPI_SENS_CS_Pin GPIO_PIN_2
#define SPI_SENS_CS_GPIO_Port GPIOD
#define BOARD_ID_01_Pin GPIO_PIN_4
#define BOARD_ID_01_GPIO_Port GPIOB
#define BOARD_ID_02_Pin GPIO_PIN_5
#define BOARD_ID_02_GPIO_Port GPIOB
#define LAM3_EN_Pin GPIO_PIN_7
#define LAM3_EN_GPIO_Port GPIOB
#define LAM2_EN_Pin GPIO_PIN_8
#define LAM2_EN_GPIO_Port GPIOB
#define LAM1_EN_Pin GPIO_PIN_9
#define LAM1_EN_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
