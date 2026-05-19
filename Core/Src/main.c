/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Main program body
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
#include "main.h"
#include "adc.h"
#include "spi.h"
#include "tim.h"
#include "usart.h"
#include "gpio.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "ir_sensor.h"      // ← 加這行，引入 IR 感測器模組的標頭，取得常數、extern 變數宣告與 API 原型
#include <stdio.h>      // ⭐ snprintf
#include <string.h>     // ⭐ strlen（這個其實用不到，但帶著保險）
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

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

/* USER CODE END 0 */

/**
  * @brief  The application entry point.
  * @retval int
  */
int main(void)
{

  /* USER CODE BEGIN 1 */

  /* USER CODE END 1 */

  /* MCU Configuration--------------------------------------------------------*/

  /* Reset of all peripherals, Initializes the Flash interface and the Systick. */
  HAL_Init();

  /* USER CODE BEGIN Init */

  /* USER CODE END Init */

  /* Configure the system clock */
  SystemClock_Config();

  /* USER CODE BEGIN SysInit */

  /* USER CODE END SysInit */

  /* Initialize all configured peripherals */
  MX_GPIO_Init();
  MX_ADC1_Init();
  MX_SPI2_Init();
  MX_TIM1_Init();
  MX_TIM2_Init();
  MX_TIM3_Init();
  MX_TIM4_Init();
  MX_TIM8_Init();
  MX_USART3_UART_Init();
  MX_TIM10_Init();
  /* USER CODE BEGIN 2 */
  IR_Init();
  /* === 開機校正流程 === */
  // LED_1 亮起，提示「現在開始校正，請把車身在黑線上方掃動 5 秒」
  HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_SET);
  
  uint32_t cal_start = HAL_GetTick();
  while (HAL_GetTick() - cal_start < 5000)   // 5 秒
  {
      IR_Calibrate();   // 不斷讀值並更新 Max/Min
      HAL_Delay(10);    // 100 Hz 校正取樣
  }
  
  HAL_GPIO_WritePin(LED_1_GPIO_Port, LED_1_Pin, GPIO_PIN_RESET);  // LED 熄滅 = 校正完成
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */

    /*第三版測試 : 模擬實際循跡時的真實時序，讀值送到 VOFA+，並觀察正規化後的輸出是否穩定在 0~1000 範圍內 */
    IR_ReadHead();      // 1. 讀車頭原始值
    IR_Normalize();     // 2. ⭐ 立刻正規化（給後續 PID/marker 用）
    IR_ReadSide();      // 3. 讀側邊（marker 偵測用，不正規化）

    /* === VOFA+ 輸出正規化後的值，觀察是否每顆 IR 都能 0~1000 全範圍變化 === */
    char buf[128];
    int len = snprintf(buf, sizeof(buf),
                       "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
                       IR_Nor[0],  IR_Nor[1],  IR_Nor[2],  IR_Nor[3],  IR_Nor[4],
                       IR_Nor[5],  IR_Nor[6],  IR_Nor[7],  IR_Nor[8],  IR_Nor[9],
                       IR_Nor[10], IR_Nor[11], IR_Nor[12]);

    HAL_UART_Transmit(&huart3, (uint8_t*)buf, len, 100);

    HAL_Delay(20);  // 50 Hz 更新率，VOFA+ 顯示流暢度剛好

    /*第二版測試：分開讀車頭/側邊，模擬實際循跡時的真實時序，並把讀值送到 VOFA+ */
    /* ⭐ 分開讀，避免 HEAD/SIDE 互相干擾，
     *    這也比較接近實際循跡時的真實時序 */
    
    /* IR_ReadHead();   // 只開 HEAD MOSFET → 讀 IR[1]~IR[11]
    IR_ReadSide();   // 只開 SIDE MOSFET → 讀 IR[0], IR[12]
    */
    /* 兩次讀取之間中間有極短的「兩邊都暗」空檔，
     * 對 ADC 讀值無害，反而能讓 LED 散熱 */

    /* === 組裝 FireWater 字串：13 個值 === */
    /*char buf[128];
    int len = snprintf(buf, sizeof(buf),
                       "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
                       IR[0],  IR[1],  IR[2],  IR[3],  IR[4],
                       IR[5],  IR[6],  IR[7],  IR[8],  IR[9],
                       IR[10], IR[11], IR[12]);

    HAL_UART_Transmit(&huart3, (uint8_t*)buf, len, 100);

    HAL_Delay(20);  // 50 Hz 更新率，VOFA+ 顯示流暢度剛好
    */

    /*第一版測試：一次讀全部 13 顆，方便 debug 時觀察所有值
     * IR_ReadAll();        // 一次讀 13 顆，方便 debug 時觀察所有值
     * HAL_Delay(100);      // 100ms 讀一次，debug 期間夠用了
     */
    /*
    IR_ReadAll();   // 讀 13 顆 IR
    */
    /* === 組裝 FireWater 字串：val1,val2,...,val13\n === */
    /*
    char buf[128];                                                  // 13 個 4 位數 + 12 個逗號 + \n + 餘裕 ≈ 65 bytes，128 安全
    int len = snprintf(buf, sizeof(buf),
                       "%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u,%u\n",
                       IR[0],  IR[1],  IR[2],  IR[3],  IR[4],
                       IR[5],  IR[6],  IR[7],  IR[8],  IR[9],
                       IR[10], IR[11], IR[12]);

    HAL_UART_Transmit(&huart3, (uint8_t*)buf, len, 100);             // 阻塞式傳送，timeout 100ms

    HAL_Delay(20);  // 50 Hz 更新率，VOFA+ 顯示流暢度剛好
    */
    /*
    IR_ReadAll();        // 一次讀 13 顆，方便 debug 時觀察所有值
    HAL_Delay(100);      // 100ms 讀一次，debug 期間夠用了
    */
    }
  /* USER CODE END 3 */
}

/**
  * @brief System Clock Configuration
  * @retval None
  */
void SystemClock_Config(void)
{
  RCC_OscInitTypeDef RCC_OscInitStruct = {0};
  RCC_ClkInitTypeDef RCC_ClkInitStruct = {0};

  /** Configure the main internal regulator output voltage
  */
  __HAL_RCC_PWR_CLK_ENABLE();
  __HAL_PWR_VOLTAGESCALING_CONFIG(PWR_REGULATOR_VOLTAGE_SCALE1);

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLM = 8;
  RCC_OscInitStruct.PLL.PLLN = 168;
  RCC_OscInitStruct.PLL.PLLP = RCC_PLLP_DIV2;
  RCC_OscInitStruct.PLL.PLLQ = 4;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_PLLCLK;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV4;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV2;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_5) != HAL_OK)
  {
    Error_Handler();
  }
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/**
  * @brief  This function is executed in case of error occurrence.
  * @retval None
  */
void Error_Handler(void)
{
  /* USER CODE BEGIN Error_Handler_Debug */
  /* User can add his own implementation to report the HAL error return state */
  __disable_irq();
  while (1)
  {
  }
  /* USER CODE END Error_Handler_Debug */
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
