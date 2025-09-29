/* USER CODE BEGIN Header */
/**
  ******************************************************************************
  * @file           : main.c
  * @brief          : Countdown Timer with LCD + Keypad + Buzzer
  ******************************************************************************
  * @attention
  * Copyright (c) 2025
  * All rights reserved.
  ******************************************************************************
  */
/* USER CODE END Header */
/* Includes ------------------------------------------------------------------*/
#include "main.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "lcd_i2c.h"
#include "keypad.h"
#include <stdio.h>
#include <string.h>
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
    STATE_SET_TIME,
    STATE_COUNTDOWN,
    STATE_ALARM
} AppState;
/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
I2C_HandleTypeDef hi2c1;

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */
AppState current_state = STATE_SET_TIME;
int countdown_seconds = 0;
char input_buffer[7] = "000000"; // Lưu HHMMSS
int input_index = 0;

int last_seconds = -1;    // Dùng để tránh update LCD liên tục
int buzzer_toggle = 0;    // Dùng để beep beep trong STATE_ALARM
/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */
void handle_set_time_state(void);
void handle_countdown_state(void);
void handle_alarm_state(void);
void format_time_string(char *dest, const char *src);
/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */

// Hàm chuyển input_buffer "HHMMSS" thành chuỗi "HH:MM:SS"
void format_time_string(char *dest, const char *src) {
    sprintf(dest, "%c%c:%c%c:%c%c",
            src[0], src[1],
            src[2], src[3],
            src[4], src[5]);
}

void handle_set_time_state(void) {
    char display_str[12];

    LCD_SetCursor(0, 0);
    LCD_PrintString("SET TIME:       "); // Dùng space để xóa ký tự thừa nếu có

    format_time_string(display_str, input_buffer);
    LCD_SetCursor(1, 0);
    LCD_PrintString(display_str);

    // Hiển thị con trỏ tại vị trí nhập liệu
    if(input_index < 6) {
        LCD_SetCursor(1, input_index + (input_index / 2)); // Điều chỉnh vị trí do có dấu ":"
        LCD_PrintString("_"); 
    }

    char key = Keypad_GetKey();
    if (key >= '0' && key <= '9') {
        if (input_index < 6) {
            input_buffer[input_index++] = key;
        }
    } else if (key == '#') { // Nhấn # để xác nhận
        int hours = (input_buffer[0] - '0') * 10 + (input_buffer[1] - '0');
        int minutes = (input_buffer[2] - '0') * 10 + (input_buffer[3] - '0');
        int seconds = (input_buffer[4] - '0') * 10 + (input_buffer[5] - '0');

        countdown_seconds = hours * 3600 + minutes * 60 + seconds;

        if (countdown_seconds > 0) {
            current_state = STATE_COUNTDOWN;
            HAL_TIM_Base_Start_IT(&htim2); // Bắt đầu timer cho đếm ngược
            LCD_Clear();
        } else {
            LCD_Clear();
            LCD_SetCursor(0, 0);
            LCD_PrintString("Invalid Time!");
            HAL_Delay(1000);
            strcpy(input_buffer, "000000");
            input_index = 0;
            LCD_Clear();
        }
    } else if (key == '*') { // Nhấn * để xóa
        if (input_index > 0) {
            input_buffer[--input_index] = '0';
        }
    }
}

void handle_countdown_state(void) {
    // Chỉ cập nhật LCD khi số giây thay đổi
    if (countdown_seconds != last_seconds) {
        last_seconds = countdown_seconds;

        int hours = countdown_seconds / 3600;
        int minutes = (countdown_seconds % 3600) / 60;
        int seconds = countdown_seconds % 60;

        char time_str[12];
        sprintf(time_str, "%02d:%02d:%02d", hours, minutes, seconds);

        LCD_SetCursor(0, 0);
        LCD_PrintString("COUNTDOWN       ");
        LCD_SetCursor(1, 0);
        LCD_PrintString(time_str);
    }
    
    // Kiểm tra bàn phím nếu muốn thêm tính năng PAUSE/STOP
    // Ví dụ: if (Keypad_GetKey() == 'A') current_state = STATE_SET_TIME;
}

void handle_alarm_state(void) {
    // Beep beep: bật/tắt buzzer mỗi 500ms (được xử lý trong ngắt Timer)
    // Buzzer_toggle được đảo trạng thái trong HAL_TIM_PeriodElapsedCallback

    LCD_SetCursor(0, 0);
    LCD_PrintString(" TIME'S UP!     ");
    LCD_SetCursor(1, 0);
    LCD_PrintString("Press any key   "); // Dùng space để xóa ký tự thừa

    if (Keypad_GetKey() != 0) {
        HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET); // Tắt buzzer
        current_state = STATE_SET_TIME; // Chuyển về trạng thái SET_TIME
        countdown_seconds = 0;
        input_index = 0;
        strcpy(input_buffer, "000000");
        last_seconds = -1;
        LCD_Clear();
    }
}

// Ngắt Timer mỗi 1 giây
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim) {
    if (htim->Instance == TIM2) {
        if (current_state == STATE_COUNTDOWN) {
            if (countdown_seconds > 0) {
                countdown_seconds--;
            } else {
                // KHÔNG dừng timer, chỉ chuyển trạng thái
current_state = STATE_ALARM;
                current_state = STATE_ALARM; // Chuyển sang ALARM khi hết giờ
            }
        } else if (current_state == STATE_ALARM) {
            buzzer_toggle ^= 1; // Đảo trạng thái buzzer_toggle (0 -> 1 -> 0...)
            
            // Dựa vào buzzer_toggle để bật/tắt chân BUZZER
            if (buzzer_toggle) {
                HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_SET);
            } else {
                HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);
            }
        }
    }
}
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
  MX_I2C1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */

  // ******************************************************
  // BƯỚC SỬA 1: KHỞI TẠO LCD
  // ******************************************************
  LCD_Init(&hi2c1);
  LCD_Clear();
  
  // Khởi tạo trạng thái bàn phím (nếu cần)
  // Keypad_Init(); 
  
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
    // ******************************************************
    // BƯỚC SỬA 2: LÕI MÁY TRẠNG THÁI (STATE MACHINE)
    // ******************************************************
    switch (current_state) {
        case STATE_SET_TIME:
            handle_set_time_state();
            break;
        case STATE_COUNTDOWN:
            handle_countdown_state();
            break;
        case STATE_ALARM:
            handle_alarm_state();
            break;
    }
    
    // Thêm một độ trễ ngắn để giảm tải CPU và cho phép Keypad hoạt động ổn định
    HAL_Delay(50);
    
    /* USER CODE END WHILE */

    /* USER CODE BEGIN 3 */
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

  /** Initializes the RCC Oscillators according to the specified parameters
  * in the RCC_OscInitTypeDef structure.
  */
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSE;
  RCC_OscInitStruct.HSEState = RCC_HSE_ON;
  RCC_OscInitStruct.HSEPredivValue = RCC_HSE_PREDIV_DIV1;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_ON;
  RCC_OscInitStruct.PLL.PLLSource = RCC_PLLSOURCE_HSE;
  RCC_OscInitStruct.PLL.PLLMUL = RCC_PLL_MUL9;
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
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV2;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_2) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief I2C1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_I2C1_Init(void)
{

  /* USER CODE BEGIN I2C1_Init 0 */

  /* USER CODE END I2C1_Init 0 */

  /* USER CODE BEGIN I2C1_Init 1 */

  /* USER CODE END I2C1_Init 1 */
  hi2c1.Instance = I2C1;
  hi2c1.Init.ClockSpeed = 100000;
  hi2c1.Init.DutyCycle = I2C_DUTYCYCLE_2;
  hi2c1.Init.OwnAddress1 = 0;
  hi2c1.Init.AddressingMode = I2C_ADDRESSINGMODE_7BIT;
  hi2c1.Init.DualAddressMode = I2C_DUALADDRESS_DISABLE;
  hi2c1.Init.OwnAddress2 = 0;
  hi2c1.Init.GeneralCallMode = I2C_GENERALCALL_DISABLE;
  hi2c1.Init.NoStretchMode = I2C_NOSTRETCH_DISABLE;
  if (HAL_I2C_Init(&hi2c1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN I2C1_Init 2 */

  /* USER CODE END I2C1_Init 2 */

}

/**
  * @brief TIM2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_TIM2_Init(void)
{

  /* USER CODE BEGIN TIM2_Init 0 */

  /* USER CODE END TIM2_Init 0 */

  TIM_ClockConfigTypeDef sClockSourceConfig = {0};
  TIM_MasterConfigTypeDef sMasterConfig = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7199;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 9999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_Base_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sClockSourceConfig.ClockSource = TIM_CLOCKSOURCE_INTERNAL;
  if (HAL_TIM_ConfigClockSource(&htim2, &sClockSourceConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */

}

/**
  * @brief GPIO Initialization Function
  * @param None
  * @retval None
  */
static void MX_GPIO_Init(void)
{
  GPIO_InitTypeDef GPIO_InitStruct = {0};
  /* USER CODE BEGIN MX_GPIO_Init_1 */

  /* USER CODE END MX_GPIO_Init_1 */

  /* GPIO Ports Clock Enable */
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, KEYPAD_ROW_1_Pin|KEYPAD_ROW_2_Pin|KEYPAD_ROW_3_Pin|KEYPAD_ROW_4_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BUZZER_GPIO_Port, BUZZER_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pins : KEYPAD_ROW_1_Pin KEYPAD_ROW_2_Pin KEYPAD_ROW_3_Pin KEYPAD_ROW_4_Pin */
  GPIO_InitStruct.Pin = KEYPAD_ROW_1_Pin|KEYPAD_ROW_2_Pin|KEYPAD_ROW_3_Pin|KEYPAD_ROW_4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : KEYPAD_COL_1_Pin KEYPAD_COL_2_Pin KEYPAD_COL_3_Pin KEYPAD_COL_4_Pin */
  GPIO_InitStruct.Pin = KEYPAD_COL_1_Pin|KEYPAD_COL_2_Pin|KEYPAD_COL_3_Pin|KEYPAD_COL_4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pin : BUZZER_Pin */
  GPIO_InitStruct.Pin = BUZZER_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BUZZER_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
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
