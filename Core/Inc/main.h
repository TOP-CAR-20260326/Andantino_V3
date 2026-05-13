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
#include "stm32f4xx_hal.h"

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
#define LED_3_Pin GPIO_PIN_13
#define LED_3_GPIO_Port GPIOC
#define LED_2_Pin GPIO_PIN_14
#define LED_2_GPIO_Port GPIOC
#define LED_1_Pin GPIO_PIN_15
#define LED_1_GPIO_Port GPIOC
#define IR_R_Pin GPIO_PIN_0
#define IR_R_GPIO_Port GPIOC
#define IR11_Pin GPIO_PIN_1
#define IR11_GPIO_Port GPIOC
#define IR10_Pin GPIO_PIN_2
#define IR10_GPIO_Port GPIOC
#define IR9_Pin GPIO_PIN_3
#define IR9_GPIO_Port GPIOC
#define IR8_Pin GPIO_PIN_0
#define IR8_GPIO_Port GPIOA
#define IR7_Pin GPIO_PIN_1
#define IR7_GPIO_Port GPIOA
#define IR6_Pin GPIO_PIN_2
#define IR6_GPIO_Port GPIOA
#define IR5_Pin GPIO_PIN_3
#define IR5_GPIO_Port GPIOA
#define IR4_Pin GPIO_PIN_4
#define IR4_GPIO_Port GPIOA
#define IR3_Pin GPIO_PIN_5
#define IR3_GPIO_Port GPIOA
#define IR2_Pin GPIO_PIN_6
#define IR2_GPIO_Port GPIOA
#define IR1_Pin GPIO_PIN_7
#define IR1_GPIO_Port GPIOA
#define IR_MOS_HEAD_Pin GPIO_PIN_4
#define IR_MOS_HEAD_GPIO_Port GPIOC
#define IR_L_Pin GPIO_PIN_5
#define IR_L_GPIO_Port GPIOC
#define LED_R_Pin GPIO_PIN_2
#define LED_R_GPIO_Port GPIOB
#define LED_L_Pin GPIO_PIN_10
#define LED_L_GPIO_Port GPIOB
#define BUZZER_Pin GPIO_PIN_11
#define BUZZER_GPIO_Port GPIOB
#define SPI2_CS_Pin GPIO_PIN_12
#define SPI2_CS_GPIO_Port GPIOB
#define FAN_PWM_Pin GPIO_PIN_9
#define FAN_PWM_GPIO_Port GPIOC
#define IN1_L_Pin GPIO_PIN_8
#define IN1_L_GPIO_Port GPIOA
#define IN2_L_Pin GPIO_PIN_9
#define IN2_L_GPIO_Port GPIOA
#define IN1_R_Pin GPIO_PIN_10
#define IN1_R_GPIO_Port GPIOA
#define IN2_R_Pin GPIO_PIN_11
#define IN2_R_GPIO_Port GPIOA
#define BUTTON_Pin GPIO_PIN_12
#define BUTTON_GPIO_Port GPIOA
#define ENCODER_RB_Pin GPIO_PIN_4
#define ENCODER_RB_GPIO_Port GPIOB
#define ENCODER_RA_Pin GPIO_PIN_5
#define ENCODER_RA_GPIO_Port GPIOB
#define ENCODER_LA_Pin GPIO_PIN_6
#define ENCODER_LA_GPIO_Port GPIOB
#define ENCODER_LB_Pin GPIO_PIN_7
#define ENCODER_LB_GPIO_Port GPIOB
#define IR_MOS_SIDE_Pin GPIO_PIN_9
#define IR_MOS_SIDE_GPIO_Port GPIOB

/* USER CODE BEGIN Private defines */

/* USER CODE END Private defines */

#ifdef __cplusplus
}
#endif

#endif /* __MAIN_H */
