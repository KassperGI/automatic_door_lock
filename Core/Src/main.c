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

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include "i2c-lcd.h"
#include <string.h>
#include <stdio.h>
#include "rc522.h"
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
I2C_HandleTypeDef hi2c1;

SPI_HandleTypeDef hspi1;

TIM_HandleTypeDef htim2;

/* USER CODE BEGIN PV */

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_SPI1_Init(void);
static void MX_TIM2_Init(void);
/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
#define FLASH_ADDR 0x08007C00 // The absolute end of the 32KB C6 flash

// Create a package for our security data
typedef struct {
    uint32_t memory_flag; // A special tag to know if we've saved before
    char pin[8];
    int total;
    uint8_t cards[5][4];
} SaveData;

void load_from_flash(char* pin, int* total, uint8_t cards[5][4]) {
    SaveData* saved = (SaveData*)FLASH_ADDR;
    // If the memory flag matches, load the saved data!
    if(saved->memory_flag == 0xABCD1234) {
        strcpy(pin, saved->pin);
        *total = saved->total;
        memcpy(cards, saved->cards, 20);
    }
}

void save_to_flash(char* pin, int total, uint8_t cards[5][4]) {
    SaveData data;
    data.memory_flag = 0xABCD1234;
    strcpy(data.pin, pin);
    data.total = total;
    memcpy(data.cards, cards, 20);

    HAL_FLASH_Unlock();

    // Step 1: Erase the old page
    FLASH_EraseInitTypeDef EraseInitStruct;
    uint32_t PageError = 0;
    EraseInitStruct.TypeErase = FLASH_TYPEERASE_PAGES;
    EraseInitStruct.PageAddress = FLASH_ADDR;
    EraseInitStruct.NbPages = 1;
    HAL_FLASHEx_Erase(&EraseInitStruct, &PageError);

    // Step 2: Write the new data package into the silicon
    uint32_t* ptr = (uint32_t*)&data;
    for(int i = 0; i < sizeof(SaveData)/4; i++) {
        HAL_FLASH_Program(FLASH_TYPEPROGRAM_WORD, FLASH_ADDR + (i*4), ptr[i]);
    }

    HAL_FLASH_Lock();
}
void beep_success(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET); HAL_Delay(100);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); HAL_Delay(150);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
}

void beep_error(void) {
    for(int i=0; i<3; i++) {
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); HAL_Delay(80);
        HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET); HAL_Delay(80);
    }
}

void beep_lock(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); HAL_Delay(400);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
}

void beep_mode(void) {
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); HAL_Delay(40);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET); HAL_Delay(40);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); HAL_Delay(40);
    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
}
char read_keypad(void)
{
    GPIO_TypeDef* row_ports[] = {GPIOB, GPIOB, GPIOB, GPIOB};
    uint16_t row_pins[] = {GPIO_PIN_12, GPIO_PIN_13, GPIO_PIN_14, GPIO_PIN_15};
    GPIO_TypeDef* col_ports[] = {GPIOA, GPIOA, GPIOA, GPIOA};
    uint16_t col_pins[] = {GPIO_PIN_8, GPIO_PIN_9, GPIO_PIN_10, GPIO_PIN_15};

    char keys[4][4] = {
        {'1', '2', '3', 'A'},
        {'4', '5', '6', 'B'},
        {'7', '8', '9', 'C'},
        {'*', '0', '#', 'D'}
    };

    for (int r = 0; r < 4; r++)
    {
        HAL_GPIO_WritePin(row_ports[r], row_pins[r], GPIO_PIN_RESET);

        for (int c = 0; c < 4; c++)
        {
            if (HAL_GPIO_ReadPin(col_ports[c], col_pins[c]) == GPIO_PIN_RESET)
            {
                HAL_Delay(200); // Safe delay instead of an infinite trap loop
                HAL_GPIO_WritePin(row_ports[r], row_pins[r], GPIO_PIN_SET);
                return keys[r][c];
            }
        }
        HAL_GPIO_WritePin(row_ports[r], row_pins[r], GPIO_PIN_SET);
    }
    return '\0';
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
  MX_SPI1_Init();
  MX_TIM2_Init();
  /* USER CODE BEGIN 2 */
  lcd_init();
  lcd_send_string("DOOR LOCKED");
  lcd_put_cur(1, 0);
  lcd_send_string("Enter PIN: ");
  // Initialize RFID
  MFRC522_Init();
  // Initialize all rows to HIGH
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_12 | GPIO_PIN_13 | GPIO_PIN_14 | GPIO_PIN_15, GPIO_PIN_SET);
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);
  // Start the servo motor PWM signal
  HAL_TIM_PWM_Start(&htim2, TIM_CHANNEL_1);
  TIM2->CCR1 = 500; // 500 = 0 degrees (Locked Position)
  /* USER CODE END 2 */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  int blink_timer = 0;
  uint32_t unlock_timer = 0;
  uint32_t sleep_timer = HAL_GetTick();
  int is_asleep = 0;

    char current_pin[5] = "0000";
    char input_buffer[5] = "";
    int input_index = 0;
    int system_state = 0; // 0=Locked, 1=Unlocked, 2=ChangePIN, 3=VerifyPIN for Card, 4=ScanNewCard

    // Database for up to 5 authorized cards. (Slot 0 is your master card).
    uint8_t authorized_cards[5][4] = {
        {0x5B, 0x93, 0x6A, 0x06},
        {0}, {0}, {0}, {0}
    };
    int total_cards = 1;
    load_from_flash(current_pin, &total_cards, authorized_cards);

    while (1)
    {
        // 1. Check for RFID Card First
        uint8_t rfid_id[5];
        if (MFRC522_Check(rfid_id) == 0)
        {
        	sleep_timer = HAL_GetTick(); // Reset the 15-second clock
        	          if (is_asleep) {
        	              is_asleep = 0;
        	              uint8_t wake_cmd = 0x08;
        	              HAL_I2C_Master_Transmit(&hi2c1, 0x4E, &wake_cmd, 1, 10);
        	          }
            // IF WE ARE IN "ADD CARD" MODE (State 4)
            if (system_state == 4)
            {
                if (total_cards < 5)
                {
                    // Save the new card to the next empty slot
                    authorized_cards[total_cards][0] = rfid_id[0];
                    authorized_cards[total_cards][1] = rfid_id[1];
                    authorized_cards[total_cards][2] = rfid_id[2];
                    authorized_cards[total_cards][3] = rfid_id[3];
                    total_cards++;

                    save_to_flash(current_pin, total_cards, authorized_cards);
                    beep_success();
                    lcd_clear();
                    lcd_put_cur(0, 2);
                    lcd_send_string("CARD ADDED!");
                }
                else
                {
                    beep_error();
                    lcd_clear();
                    lcd_put_cur(0, 2);
                    lcd_send_string("MEMORY FULL!");
                }

                HAL_Delay(1500);
                // Return to the Unlocked Menu
                system_state = 1;
                TIM2->CCR1 = 1500; // 1500 = 90 degrees (Unlocked Position)
                lcd_clear();
                lcd_put_cur(0, 1);
                lcd_send_string("DOOR UNLOCKED!");
                lcd_put_cur(1, 0);
                lcd_send_string("#:Lck *:PIN A:ID");
            }
            // IF WE ARE IN "LOCKED" MODE (State 0)
            else if (system_state == 0)
            {
                int card_matched = 0;
                // Check the scanned card against all saved cards
                for (int i = 0; i < total_cards; i++)
                {
                    if (rfid_id[0] == authorized_cards[i][0] && rfid_id[1] == authorized_cards[i][1] &&
                        rfid_id[2] == authorized_cards[i][2] && rfid_id[3] == authorized_cards[i][3])
                    {
                        card_matched = 1;
                        break;
                    }
                }

                if (card_matched == 1)
                {
                    system_state = 1; // Switch to UNLOCKED
                    TIM2->CCR1 = 1500; // 1500 = 90 degrees (Unlocked Position)
                    unlock_timer = HAL_GetTick();
                    beep_success();
                    lcd_clear();
                    lcd_put_cur(0, 1);
                    lcd_send_string("DOOR UNLOCKED!");
                    lcd_put_cur(1, 0);
                    lcd_send_string("#:Lck *:PIN A:ID"); // Updated menu text
                }
                else
                {
                    beep_error();
                    lcd_clear();
                    lcd_put_cur(0, 1);
                    lcd_send_string("ACCESS DENIED!");
                    HAL_Delay(1500);

                    lcd_clear();
                    lcd_put_cur(0, 0);
                    lcd_send_string("DOOR LOCKED");
                    lcd_put_cur(1, 0);
                    lcd_send_string("Enter PIN: ");
                }
            }

            input_index = 0;
            memset(input_buffer, 0, sizeof(input_buffer));
            HAL_Delay(1000);
        }
        char key = read_keypad();

        if (key != '\0')
        {
        	sleep_timer = HAL_GetTick(); // Reset the 15-second clock
        	          if (is_asleep) {
        	              is_asleep = 0;
        	              uint8_t wake_cmd = 0x08;
        	              HAL_I2C_Master_Transmit(&hi2c1, 0x4E, &wake_cmd, 1, 10);
        	          }
            // ---------------------------------------------------------
            // STATE 0: DOOR IS LOCKED
            // ---------------------------------------------------------
            if (system_state == 0)
            {
                if (key >= '0' && key <= '9')
                {
                    // Standard key press beep
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); HAL_Delay(50);
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);

                    input_buffer[input_index] = key;
                    lcd_put_cur(1, 11 + input_index);
                    lcd_send_data('*');
                    input_index++;

                    if (input_index == 4)
                    {
                        input_buffer[4] = '\0';

                        if (strcmp(input_buffer, current_pin) == 0)
                        {
                            system_state = 1;
                            TIM2->CCR1 = 1500;
                            unlock_timer = HAL_GetTick();
                            beep_success(); // <--- SUCCESS SOUND
                            lcd_clear();
                            lcd_put_cur(0, 1);
                            lcd_send_string("DOOR UNLOCKED!");
                            lcd_put_cur(1, 0);
                            lcd_send_string("#:Lock *:NewPIN");
                        }
                        else
                        {
                            beep_error(); // <--- ERROR SOUND
                            lcd_clear();
                            lcd_put_cur(0, 3);
                            lcd_send_string("WRONG PIN!");
                            HAL_Delay(1500);

                            lcd_clear();
                            lcd_put_cur(0, 0);
                            lcd_send_string("DOOR LOCKED");
                            lcd_put_cur(1, 0);
                            lcd_send_string("Enter PIN: ");
                        }
                        input_index = 0;
                        memset(input_buffer, 0, sizeof(input_buffer));
                    }
                }
            }
            // ---------------------------------------------------------
            // STATE 1: DOOR IS UNLOCKED
            // ---------------------------------------------------------
            else if (system_state == 1)
                      {
                          if (key == '#') // Lock the door
                          {
                              beep_lock();
                              TIM2->CCR1 = 500;
                              system_state = 0;
                              input_index = 0;
                              lcd_clear();
                              lcd_put_cur(0, 0);
                              lcd_send_string("DOOR LOCKED");
                              lcd_put_cur(1, 0);
                              lcd_send_string("Enter PIN: ");
                          }
                          else if (key == '*') // Change the PIN
                          {
                              beep_mode();
                              system_state = 2;
                              input_index = 0;
                              memset(input_buffer, 0, sizeof(input_buffer));
                              lcd_clear();
                              lcd_put_cur(0, 1);
                              lcd_send_string("ENTER NEW PIN:");
                          }
                          else if (key == 'A') // Add a new RFID Card
                          {
                              beep_mode();
                              system_state = 3;
                              input_index = 0;
                              memset(input_buffer, 0, sizeof(input_buffer));
                              lcd_clear();
                              lcd_put_cur(0, 0);
                              lcd_send_string("ADD CARD MODE");
                              lcd_put_cur(1, 0);
                              lcd_send_string("Verify PIN: ");
                          }
                      }
            // ---------------------------------------------------------
            // STATE 2: CHANGING THE PIN
            // ---------------------------------------------------------
            else if (system_state == 2)
            {
                if (key >= '0' && key <= '9')
                {
                    // Standard key press beep
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET); HAL_Delay(50);
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET);

                    input_buffer[input_index] = key;
                    lcd_put_cur(1, 6 + input_index);
                    lcd_send_data(key);
                    input_index++;

                    if (input_index == 4)
                    {
                        input_buffer[4] = '\0';
                        strcpy(current_pin, input_buffer);
                        save_to_flash(current_pin, total_cards, authorized_cards);
                        beep_success(); // <--- SUCCESS SOUND
                        lcd_clear();
                        lcd_put_cur(0, 1);
                        lcd_send_string("PIN SAVED!");
                        HAL_Delay(1500);

                        system_state = 1;
                        input_index = 0;
                        lcd_clear();
                        lcd_put_cur(0, 1);
                        lcd_send_string("DOOR UNLOCKED!");
                        lcd_put_cur(1, 0);
                        lcd_send_string("#:Lock *:NewPIN");
                    }
                }
            }
            // ---------------------------------------------------------
            // STATE 3: VERIFY PIN TO ADD CARD
            // ---------------------------------------------------------
            else if (system_state == 3)
            {
                if (key >= '0' && key <= '9')
                {
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_RESET); HAL_Delay(50);
                    HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1, GPIO_PIN_SET);

                    input_buffer[input_index] = key;
                    lcd_put_cur(1, 12 + input_index);
                    lcd_send_data('*'); // Hide the entry
                    input_index++;

                    if (input_index == 4)
                    {
                        input_buffer[4] = '\0';

                        if (strcmp(input_buffer, current_pin) == 0)
                        {
                           // PIN Correct! Move to State 4 to await the card
                           system_state = 4;
                           beep_success();
                           lcd_clear();
                           lcd_put_cur(0, 2);
                           lcd_send_string("PIN ACCEPTED");
                           lcd_put_cur(1, 0);
                           lcd_send_string("Scan New Card...");
                        }
                        else
                        {
                           // PIN Incorrect! Boot back to unlocked menu
                           beep_error();
                           lcd_clear();
                           lcd_put_cur(0, 3);
                           lcd_send_string("WRONG PIN!");
                           HAL_Delay(1500);

                           system_state = 1;
                           lcd_clear();
                           lcd_put_cur(0, 1);
                           lcd_send_string("DOOR UNLOCKED!");
                           lcd_put_cur(1, 0);
                           lcd_send_string("#:Lck *:PIN A:ID");
                        }
                        input_index = 0;
                        memset(input_buffer, 0, sizeof(input_buffer));
                    }
                }
            }
        }
        // ---------------------------------------------------------
        // AUTO-LOCK TIMER
        // ---------------------------------------------------------
        if (system_state == 1 && (HAL_GetTick() - unlock_timer >= 5000))
        {
            beep_lock(); // Make the locking sound
            TIM2->CCR1 = 500; // Move back to Locked Position
            system_state = 0;
            input_index = 0;
            memset(input_buffer, 0, sizeof(input_buffer));
            lcd_clear();
            lcd_put_cur(0, 0);
            lcd_send_string("DOOR LOCKED");
            lcd_put_cur(1, 0);
            lcd_send_string("Enter PIN: ");
        }
        // ---------------------------------------------------------
        // SCREEN SLEEP TIMER (15 Seconds)
        // ---------------------------------------------------------
        if (!is_asleep && (HAL_GetTick() - sleep_timer >= 15000))
        {
            is_asleep = 1;
            uint8_t sleep_cmd = 0x00;
            HAL_I2C_Master_Transmit(&hi2c1, 0x4E, &sleep_cmd, 1, 10); // Cut backlight power
        }

        blink_timer++;
        if (blink_timer >= 50)
        {
            HAL_GPIO_TogglePin(GPIOC, GPIO_PIN_13);
            blink_timer = 0;
        }
        HAL_Delay(10);
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
  RCC_OscInitStruct.OscillatorType = RCC_OSCILLATORTYPE_HSI;
  RCC_OscInitStruct.HSIState = RCC_HSI_ON;
  RCC_OscInitStruct.HSICalibrationValue = RCC_HSICALIBRATION_DEFAULT;
  RCC_OscInitStruct.PLL.PLLState = RCC_PLL_NONE;
  if (HAL_RCC_OscConfig(&RCC_OscInitStruct) != HAL_OK)
  {
    Error_Handler();
  }

  /** Initializes the CPU, AHB and APB buses clocks
  */
  RCC_ClkInitStruct.ClockType = RCC_CLOCKTYPE_HCLK|RCC_CLOCKTYPE_SYSCLK
                              |RCC_CLOCKTYPE_PCLK1|RCC_CLOCKTYPE_PCLK2;
  RCC_ClkInitStruct.SYSCLKSource = RCC_SYSCLKSOURCE_HSI;
  RCC_ClkInitStruct.AHBCLKDivider = RCC_SYSCLK_DIV1;
  RCC_ClkInitStruct.APB1CLKDivider = RCC_HCLK_DIV1;
  RCC_ClkInitStruct.APB2CLKDivider = RCC_HCLK_DIV1;

  if (HAL_RCC_ClockConfig(&RCC_ClkInitStruct, FLASH_LATENCY_0) != HAL_OK)
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
  * @brief SPI1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_SPI1_Init(void)
{

  /* USER CODE BEGIN SPI1_Init 0 */

  /* USER CODE END SPI1_Init 0 */

  /* USER CODE BEGIN SPI1_Init 1 */

  /* USER CODE END SPI1_Init 1 */
  /* SPI1 parameter configuration*/
  hspi1.Instance = SPI1;
  hspi1.Init.Mode = SPI_MODE_MASTER;
  hspi1.Init.Direction = SPI_DIRECTION_2LINES;
  hspi1.Init.DataSize = SPI_DATASIZE_8BIT;
  hspi1.Init.CLKPolarity = SPI_POLARITY_LOW;
  hspi1.Init.CLKPhase = SPI_PHASE_1EDGE;
  hspi1.Init.NSS = SPI_NSS_SOFT;
  hspi1.Init.BaudRatePrescaler = SPI_BAUDRATEPRESCALER_2;
  hspi1.Init.FirstBit = SPI_FIRSTBIT_MSB;
  hspi1.Init.TIMode = SPI_TIMODE_DISABLE;
  hspi1.Init.CRCCalculation = SPI_CRCCALCULATION_DISABLE;
  hspi1.Init.CRCPolynomial = 10;
  if (HAL_SPI_Init(&hspi1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN SPI1_Init 2 */

  /* USER CODE END SPI1_Init 2 */

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

  TIM_MasterConfigTypeDef sMasterConfig = {0};
  TIM_OC_InitTypeDef sConfigOC = {0};

  /* USER CODE BEGIN TIM2_Init 1 */

  /* USER CODE END TIM2_Init 1 */
  htim2.Instance = TIM2;
  htim2.Init.Prescaler = 7;
  htim2.Init.CounterMode = TIM_COUNTERMODE_UP;
  htim2.Init.Period = 19999;
  htim2.Init.ClockDivision = TIM_CLOCKDIVISION_DIV1;
  htim2.Init.AutoReloadPreload = TIM_AUTORELOAD_PRELOAD_DISABLE;
  if (HAL_TIM_PWM_Init(&htim2) != HAL_OK)
  {
    Error_Handler();
  }
  sMasterConfig.MasterOutputTrigger = TIM_TRGO_RESET;
  sMasterConfig.MasterSlaveMode = TIM_MASTERSLAVEMODE_DISABLE;
  if (HAL_TIMEx_MasterConfigSynchronization(&htim2, &sMasterConfig) != HAL_OK)
  {
    Error_Handler();
  }
  sConfigOC.OCMode = TIM_OCMODE_PWM1;
  sConfigOC.Pulse = 0;
  sConfigOC.OCPolarity = TIM_OCPOLARITY_HIGH;
  sConfigOC.OCFastMode = TIM_OCFAST_DISABLE;
  if (HAL_TIM_PWM_ConfigChannel(&htim2, &sConfigOC, TIM_CHANNEL_1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN TIM2_Init 2 */

  /* USER CODE END TIM2_Init 2 */
  HAL_TIM_MspPostInit(&htim2);

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
  __HAL_RCC_GPIOC_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOC, GPIO_PIN_13, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, GPIO_PIN_1|GPIO_PIN_4, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOB, GPIO_PIN_0|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15, GPIO_PIN_RESET);

  /*Configure GPIO pin : PC13 */
  GPIO_InitStruct.Pin = GPIO_PIN_13;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOC, &GPIO_InitStruct);

  /*Configure GPIO pins : PA1 PA4 */
  GPIO_InitStruct.Pin = GPIO_PIN_1|GPIO_PIN_4;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : PB0 PB12 PB13 PB14
                           PB15 */
  GPIO_InitStruct.Pin = GPIO_PIN_0|GPIO_PIN_12|GPIO_PIN_13|GPIO_PIN_14
                          |GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pins : PA8 PA9 PA10 PA15 */
  GPIO_InitStruct.Pin = GPIO_PIN_8|GPIO_PIN_9|GPIO_PIN_10|GPIO_PIN_15;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

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
