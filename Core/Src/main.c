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
#include "cmsis_os.h"

/* Private includes ----------------------------------------------------------*/
/* USER CODE BEGIN Includes */
#include <stdint.h>
#include <stdio.h>
#include <stdbool.h>
#include <string.h>
#include "stm32f1xx_hal_uart.h"
#include "access_config.h"
#include "access_control.h"
#include "door_hardware.h"
#include "door_ui.h"
#include "i2c_bus.h"
#include "keypad.h"
#include "pn532.h"
#include "ssd1306.h"
/* USER CODE END Includes */

/* Private typedef -----------------------------------------------------------*/
/* USER CODE BEGIN PTD */
typedef enum {
  DOOR_EVENT_PIN_RESULT = 1,
  DOOR_EVENT_NFC_RESULT,
} DoorEventType;

typedef enum {
  PIN_MODE_NONE = 0,
  PIN_MODE_UNLOCK,
  PIN_MODE_ADMIN_AUTH,
  PIN_MODE_CHANGE_OLD,
  PIN_MODE_CHANGE_NEW,
  PIN_MODE_CHANGE_CONFIRM
} PinEntryMode;

typedef struct {
  DoorEventType type;
  bool authorized;
  uint8_t uid[PN532_MAX_UID_LENGTH];
  uint8_t uid_len;
  char pin[ACCESS_CONFIG_MAX_PIN_LENGTH + 1U];
  AccessControl_Role card_role;
} DoorEvent;

/* USER CODE END PTD */

/* Private define ------------------------------------------------------------*/
/* USER CODE BEGIN PD */


#define STATUS_LED_ON GPIO_PIN_SET
#define STATUS_LED_OFF GPIO_PIN_RESET

#define AUTH_FAILURE_ALERT_THRESHOLD 3U
#define AUTH_ALERT_COOLDOWN_MS 30000U

/* USER CODE END PD */

/* Private macro -------------------------------------------------------------*/
/* USER CODE BEGIN PM */

/* USER CODE END PM */

/* Private variables ---------------------------------------------------------*/
ADC_HandleTypeDef hadc1;

I2C_HandleTypeDef hi2c1;

UART_HandleTypeDef huart1;
UART_HandleTypeDef huart2;

/* Definitions for PasswordTask */
osThreadId_t PasswordTaskHandle;
const osThreadAttr_t PasswordTask_attributes = {
  .name = "PasswordTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for NfcTask */
osThreadId_t NfcTaskHandle;
const osThreadAttr_t NfcTask_attributes = {
  .name = "NfcTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityNormal,
};
/* Definitions for CtrlTask */
osThreadId_t CtrlTaskHandle;
const osThreadAttr_t CtrlTask_attributes = {
  .name = "CtrlTask",
  .stack_size = 256 * 4,
  .priority = (osPriority_t) osPriorityAboveNormal,
};
/* Definitions for DoorEventQueue */
osMessageQueueId_t DoorEventQueueHandle;
const osMessageQueueAttr_t DoorEventQueue_attributes = {
  .name = "DoorEventQueue"
};
/* USER CODE BEGIN PV */
static PN532_HandleTypeDef hpn532;
static SSD1306_HandleTypeDef holed;
static DoorUI_HandleTypeDef door_ui;
static Keypad_HandleTypeDef keypad;
static AccessControl access_control;
static osMutexId_t I2cBusMutexHandle;
static const osMutexAttr_t I2cBusMutex_attributes = {
  .name = "I2cBusMutex"
};
static PinEntryMode pin_entry_mode;
static bool card_enroll_active;
static char pending_new_pin[ACCESS_CONFIG_MAX_PIN_LENGTH + 1U];
static uint8_t consecutive_auth_failures;
static uint32_t last_capture_alert_tick;

/* USER CODE END PV */

/* Private function prototypes -----------------------------------------------*/
void SystemClock_Config(void);
static void MX_GPIO_Init(void);
static void MX_I2C1_Init(void);
static void MX_USART1_UART_Init(void);
static void MX_USART2_UART_Init(void);
static void MX_ADC1_Init(void);
void StartPasswordTask(void *argument);
void StartNfcTask(void *argument);
void StartCtrlTask(void *argument);

/* USER CODE BEGIN PFP */

/* USER CODE END PFP */

/* Private user code ---------------------------------------------------------*/
/* USER CODE BEGIN 0 */
static void StatusLed_Set(GPIO_PinState state)
{
  HAL_GPIO_WritePin(BOARD_LED_GPIO_Port, BOARD_LED_Pin, state);
}

static void StatusLed_Authorized(void)
{
  /* 授权成功：亮灯 500 ms。 */
  StatusLed_Set(STATUS_LED_ON);
  osDelay(500U);
  StatusLed_Set(STATUS_LED_OFF);
}

static void StatusLed_Denied(void)
{
  /* 授权失败：快速闪烁两次。 */
  for (uint8_t i = 0U; i < 2U; i++)
  {
    StatusLed_Set(STATUS_LED_ON);
    osDelay(120U);
    StatusLed_Set(STATUS_LED_OFF);
    osDelay(120U);
  }
}

static const char *NfcRoleName(AccessControl_Role role)
{
  switch (role)
  {
    case ACCESS_CONTROL_ROLE_ADMIN:
      return "ADMIN";
    case ACCESS_CONTROL_ROLE_USER:
      return "USER";
    default:
      return "STRANGER";
  }
}

static void ShowAccessResult(bool authorized)
{
  DoorUI_ShowAccessResult(&door_ui, authorized);

  if (authorized)
  {
    printf("[AUTH] AUTHORIZED\r\n");
    printf("DOOR:OPEN\r\n");
    DoorHardware_OnAccessGranted();
    printf("DOOR:CLOSED\r\n");
    StatusLed_Authorized();
  }
  else
  {
    printf("[AUTH] DENIED\r\n");
    DoorHardware_OnAccessDenied();
    StatusLed_Denied();
  }

  osDelay(500U);
  DoorUI_ShowIdle(&door_ui);
}

static bool AuthAlert_IsDue(uint32_t now)
{
  return consecutive_auth_failures >= AUTH_FAILURE_ALERT_THRESHOLD &&
         (last_capture_alert_tick == 0U ||
          (uint32_t)(now - last_capture_alert_tick) >= AUTH_ALERT_COOLDOWN_MS);
}

static void HandleAuthResult(const DoorEvent *event)
{
  uint32_t now = HAL_GetTick();

  if (event->authorized)
  {
    consecutive_auth_failures = 0U;
  }
  else if (consecutive_auth_failures < UINT8_MAX)
  {
    consecutive_auth_failures++;
  }

  /* PIN is stored as a hash; this event carries the entered PIN for host display. */
  if (event->type == DOOR_EVENT_PIN_RESULT)
  {
    printf("AUTH:PASSWORD:%s:%s:PIN\r\n",
           event->authorized ? "SUCCESS" : "FAILED",
           event->pin);
  }
  else if (event->type == DOOR_EVENT_NFC_RESULT)
  {
    char uid_str[PN532_MAX_UID_LENGTH * 2 + 1];
    for (uint8_t i = 0; i < event->uid_len; i++)
    {
      sprintf(uid_str + i * 2, "%02X", event->uid[i]);
    }
    uid_str[event->uid_len * 2] = '\0';
    printf("AUTH:NFC:%s:%s:%s\r\n",
           event->authorized ? "SUCCESS" : "FAILED",
           uid_str,
           NfcRoleName(event->card_role));
  }

  if (!event->authorized && AuthAlert_IsDue(now))
  {
    printf("[ALERT] Consecutive failures: %u\r\n", (unsigned int)consecutive_auth_failures);
    DoorHardware_OnAlert();
    last_capture_alert_tick = now;
  }

  ShowAccessResult(event->authorized);
}

static osStatus_t DoorEvent_Send(DoorEventType type, bool authorized, const uint8_t *uid, uint8_t uid_len,
                                 const char *pin, AccessControl_Role card_role)
{
  DoorEvent event = {0};

  event.type = type;
  event.authorized = authorized;
  event.uid_len = uid_len > PN532_MAX_UID_LENGTH ? PN532_MAX_UID_LENGTH : uid_len;
  event.card_role = card_role;

  if (uid != NULL && event.uid_len > 0U)
  {
    memcpy(event.uid, uid, event.uid_len);
  }

  if (pin != NULL)
  {
    strncpy(event.pin, pin, ACCESS_CONFIG_MAX_PIN_LENGTH);
    event.pin[ACCESS_CONFIG_MAX_PIN_LENGTH] = '\0';
  }

  return osMessageQueuePut(DoorEventQueueHandle, &event, 0U, 0U);
}

static bool PinEntry_IsActive(void)
{
  return pin_entry_mode != PIN_MODE_NONE;
}

static void BeginPinEntry(PinEntryMode mode, const char *title)
{
  pin_entry_mode = mode;
  DoorUI_BeginPinEntryWithTitle(&door_ui, title);
  printf("[PIN] Entry started\r\n");
}

static void EndPinEntry(void)
{
  pin_entry_mode = PIN_MODE_NONE;
  DoorUI_ShowIdle(&door_ui);
}

static void ShowDeniedMessage(const char *title)
{
  pin_entry_mode = PIN_MODE_NONE;
  DoorUI_ShowMessage(&door_ui, title, "WRONG PIN", NULL);
  DoorHardware_OnAccessDenied();
  StatusLed_Denied();
  osDelay(500U);
  DoorUI_ShowIdle(&door_ui);
}

static void SubmitPinEntry(void)
{
  const char *pin = DoorUI_GetPin(&door_ui);

  switch (pin_entry_mode)
  {
    case PIN_MODE_UNLOCK:
      pin_entry_mode = PIN_MODE_NONE;
      (void)DoorEvent_Send(DOOR_EVENT_PIN_RESULT, AccessConfig_IsPinAuthorized(pin), NULL, 0U, pin, ACCESS_CONTROL_ROLE_NONE);
      break;

    case PIN_MODE_ADMIN_AUTH:
      if (AccessConfig_IsPinAuthorized(pin))
      {
        pin_entry_mode = PIN_MODE_NONE;
        card_enroll_active = true;
        DoorUI_ShowMessage(&door_ui, "ADD CARD", "SCAN NEW CARD", "R TO CANCEL");
        printf("[ADMIN] Card enrollment started\r\n");
      }
      else
      {
        ShowDeniedMessage("ADMIN PIN");
      }
      break;

    case PIN_MODE_CHANGE_OLD:
      if (AccessConfig_IsPinAuthorized(pin))
      {
        BeginPinEntry(PIN_MODE_CHANGE_NEW, "NEW PIN");
      }
      else
      {
        ShowDeniedMessage("OLD PIN");
      }
      break;

    case PIN_MODE_CHANGE_NEW:
      if (DoorUI_GetPinLength(&door_ui) > 0U)
      {
        strncpy(pending_new_pin, pin, sizeof(pending_new_pin) - 1U);
        pending_new_pin[sizeof(pending_new_pin) - 1U] = '\0';
        BeginPinEntry(PIN_MODE_CHANGE_CONFIRM, "CONFIRM PIN");
      }
      else
      {
        DoorUI_ShowMessage(&door_ui, "NEW PIN", "1 TO 8 DIGITS", NULL);
        osDelay(700U);
        BeginPinEntry(PIN_MODE_CHANGE_NEW, "NEW PIN");
      }
      break;

    case PIN_MODE_CHANGE_CONFIRM:
      if (strcmp(pin, pending_new_pin) == 0)
      {
        (void)AccessConfig_SetPin(pin);
        pin_entry_mode = PIN_MODE_NONE;
        DoorUI_ShowMessage(&door_ui, "PIN UPDATED", "PASSWORD SAVED", NULL);
        printf("[PIN] Password changed\r\n");
        osDelay(700U);
        DoorUI_ShowIdle(&door_ui);
      }
      else
      {
        DoorUI_ShowMessage(&door_ui, "PIN MISMATCH", "TRY AGAIN", NULL);
        osDelay(700U);
        BeginPinEntry(PIN_MODE_CHANGE_NEW, "NEW PIN");
      }
      break;

    default:
      break;
  }
}

static void HandleKeypadKey(char key)
{
  if (key == KEYPAD_NO_KEY)
  {
    return;
  }

  /* PIN 输入期间不打印数字键明文，其他键正常打印 */
  if (PinEntry_IsActive() && key >= '0' && key <= '9')
  {
    printf("[KEYPAD] Key: *\r\n");
  }
  else
  {
    printf("[KEYPAD] Key: %c\r\n", key);
  }

  if (card_enroll_active)
  {
    if (key == KEYPAD_KEY_BACK)
    {
      card_enroll_active = false;
      DoorUI_ShowIdle(&door_ui);
      printf("[ADMIN] Card enrollment canceled\r\n");
    }
    return;
  }

  if (key >= '0' && key <= '9')
  {
    if (!PinEntry_IsActive())
    {
      BeginPinEntry(PIN_MODE_UNLOCK, "ENTER PIN");
    }

    (void)DoorUI_EnterPinDigit(&door_ui, key);
    return;
  }

  switch (key)
  {
    case KEYPAD_KEY_UNLOCK:
      if (PinEntry_IsActive())
      {
        SubmitPinEntry();
      }
      else
      {
        BeginPinEntry(PIN_MODE_UNLOCK, "ENTER PIN");
      }
      break;
    case KEYPAD_KEY_BACK:
      if (PinEntry_IsActive())
      {
        EndPinEntry();
      }
      break;
    case KEYPAD_KEY_ADMIN:
      BeginPinEntry(PIN_MODE_ADMIN_AUTH, "ADMIN PIN");
      break;
    case KEYPAD_KEY_CHANGE:
      BeginPinEntry(PIN_MODE_CHANGE_OLD, "OLD PIN");
      break;
    case KEYPAD_KEY_CONFIRM:
      if (PinEntry_IsActive())
      {
        SubmitPinEntry();
      }
      break;
    case KEYPAD_KEY_DELETE:
      if (PinEntry_IsActive())
      {
        (void)DoorUI_BackspacePin(&door_ui);
      }
      break;

    default:
      break;
  }
}

int _write(int file, char *ptr, int len)
{
  (void)file;
  HAL_UART_Transmit(&huart2, (uint8_t *)ptr, len, HAL_MAX_DELAY);
  return len;
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
  MX_USART1_UART_Init();
  MX_USART2_UART_Init();
  MX_ADC1_Init();
  /* USER CODE BEGIN 2 */
  AccessConfig_Init();
  AccessControl_Init(&access_control);
  printf("[AUTH] Loaded %lu authorized card(s)\r\n",
         (unsigned long)AccessConfig_LoadAuthorizedCards(&access_control));
  /* USER CODE END 2 */

  /* Init scheduler */
  osKernelInitialize();

  /* USER CODE BEGIN RTOS_MUTEX */
  /* add mutexes, ... */
  I2cBusMutexHandle = osMutexNew(&I2cBusMutex_attributes);
  if (I2cBusMutexHandle == NULL)
  {
    Error_Handler();
  }
  I2CBus_Init(I2cBusMutexHandle);
  /* USER CODE END RTOS_MUTEX */

  /* USER CODE BEGIN RTOS_SEMAPHORES */
  /* add semaphores, ... */
  /* USER CODE END RTOS_SEMAPHORES */

  /* USER CODE BEGIN RTOS_TIMERS */
  /* start timers, add new ones, ... */
  /* USER CODE END RTOS_TIMERS */

  /* Create the queue(s) */
  /* creation of DoorEventQueue */
  DoorEventQueueHandle = osMessageQueueNew (8, sizeof(DoorEvent), &DoorEventQueue_attributes);

  /* USER CODE BEGIN RTOS_QUEUES */
  /* add queues, ... */
  /* USER CODE END RTOS_QUEUES */

  /* Create the thread(s) */
  /* creation of PasswordTask */
  PasswordTaskHandle = osThreadNew(StartPasswordTask, NULL, &PasswordTask_attributes);

  /* creation of NfcTask */
  NfcTaskHandle = osThreadNew(StartNfcTask, NULL, &NfcTask_attributes);

  /* creation of CtrlTask */
  CtrlTaskHandle = osThreadNew(StartCtrlTask, NULL, &CtrlTask_attributes);

  /* USER CODE BEGIN RTOS_THREADS */
  /* add threads, ... */
  /* USER CODE END RTOS_THREADS */

  /* USER CODE BEGIN RTOS_EVENTS */
  /* add events, ... */
  /* USER CODE END RTOS_EVENTS */

  /* Start scheduler */
  osKernelStart();

  /* We should never get here as control is now taken by the scheduler */

  /* Infinite loop */
  /* USER CODE BEGIN WHILE */
  while (1)
  {
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
  RCC_PeriphCLKInitTypeDef PeriphClkInit = {0};

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
  PeriphClkInit.PeriphClockSelection = RCC_PERIPHCLK_ADC;
  PeriphClkInit.AdcClockSelection = RCC_ADCPCLK2_DIV6;
  if (HAL_RCCEx_PeriphCLKConfig(&PeriphClkInit) != HAL_OK)
  {
    Error_Handler();
  }
}

/**
  * @brief ADC1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_ADC1_Init(void)
{

  /* USER CODE BEGIN ADC1_Init 0 */

  /* USER CODE END ADC1_Init 0 */

  /* USER CODE BEGIN ADC1_Init 1 */

  /* USER CODE END ADC1_Init 1 */

  /** Common config
  */
  hadc1.Instance = ADC1;
  hadc1.Init.ScanConvMode = ADC_SCAN_DISABLE;
  hadc1.Init.ContinuousConvMode = DISABLE;
  hadc1.Init.DiscontinuousConvMode = DISABLE;
  hadc1.Init.ExternalTrigConv = ADC_SOFTWARE_START;
  hadc1.Init.DataAlign = ADC_DATAALIGN_RIGHT;
  hadc1.Init.NbrOfConversion = 1;
  if (HAL_ADC_Init(&hadc1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN ADC1_Init 2 */

  /* USER CODE END ADC1_Init 2 */

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
  * @brief USART1 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART1_UART_Init(void)
{

  /* USER CODE BEGIN USART1_Init 0 */

  /* USER CODE END USART1_Init 0 */

  /* USER CODE BEGIN USART1_Init 1 */

  /* USER CODE END USART1_Init 1 */
  huart1.Instance = USART1;
  huart1.Init.BaudRate = 115200;
  huart1.Init.WordLength = UART_WORDLENGTH_8B;
  huart1.Init.StopBits = UART_STOPBITS_1;
  huart1.Init.Parity = UART_PARITY_NONE;
  huart1.Init.Mode = UART_MODE_TX_RX;
  huart1.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart1.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart1) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART1_Init 2 */

  /* USER CODE END USART1_Init 2 */

}

/**
  * @brief USART2 Initialization Function
  * @param None
  * @retval None
  */
static void MX_USART2_UART_Init(void)
{

  /* USER CODE BEGIN USART2_Init 0 */

  /* USER CODE END USART2_Init 0 */

  /* USER CODE BEGIN USART2_Init 1 */

  /* USER CODE END USART2_Init 1 */
  huart2.Instance = USART2;
  huart2.Init.BaudRate = 115200;
  huart2.Init.WordLength = UART_WORDLENGTH_8B;
  huart2.Init.StopBits = UART_STOPBITS_1;
  huart2.Init.Parity = UART_PARITY_NONE;
  huart2.Init.Mode = UART_MODE_TX_RX;
  huart2.Init.HwFlowCtl = UART_HWCONTROL_NONE;
  huart2.Init.OverSampling = UART_OVERSAMPLING_16;
  if (HAL_UART_Init(&huart2) != HAL_OK)
  {
    Error_Handler();
  }
  /* USER CODE BEGIN USART2_Init 2 */

  /* USER CODE END USART2_Init 2 */

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
  __HAL_RCC_GPIOD_CLK_ENABLE();
  __HAL_RCC_GPIOA_CLK_ENABLE();
  __HAL_RCC_GPIOB_CLK_ENABLE();

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BOARD_LED_GPIO_Port, BOARD_LED_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(GPIOA, COL1_Pin|COL2_Pin|COL3_Pin|COL4_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(BEEF_GPIO_Port, BEEF_Pin, GPIO_PIN_RESET);

  /*Configure GPIO pin Output Level */
  HAL_GPIO_WritePin(LED_GPIO_Port, LED_Pin, GPIO_PIN_SET);

  /*Configure GPIO pin : BOARD_LED_Pin */
  GPIO_InitStruct.Pin = BOARD_LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(BOARD_LED_GPIO_Port, &GPIO_InitStruct);

  /*Configure GPIO pins : COL1_Pin COL2_Pin COL3_Pin COL4_Pin
                           BEEF_Pin */
  GPIO_InitStruct.Pin = COL1_Pin|COL2_Pin|COL3_Pin|COL4_Pin
                          |BEEF_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(GPIOA, &GPIO_InitStruct);

  /*Configure GPIO pins : ROW1_Pin ROW2_Pin ROW3_Pin ROW4_Pin */
  GPIO_InitStruct.Pin = ROW1_Pin|ROW2_Pin|ROW3_Pin|ROW4_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_INPUT;
  GPIO_InitStruct.Pull = GPIO_PULLUP;
  HAL_GPIO_Init(GPIOB, &GPIO_InitStruct);

  /*Configure GPIO pin : LED_Pin */
  GPIO_InitStruct.Pin = LED_Pin;
  GPIO_InitStruct.Mode = GPIO_MODE_OUTPUT_PP;
  GPIO_InitStruct.Pull = GPIO_NOPULL;
  GPIO_InitStruct.Speed = GPIO_SPEED_FREQ_LOW;
  HAL_GPIO_Init(LED_GPIO_Port, &GPIO_InitStruct);

  /* USER CODE BEGIN MX_GPIO_Init_2 */

  /* USER CODE END MX_GPIO_Init_2 */
}

/* USER CODE BEGIN 4 */

/* USER CODE END 4 */

/* USER CODE BEGIN Header_StartPasswordTask */
/**
  * @brief  Function implementing the PasswordTask thread.
  * @param  argument: Not used
  * @retval None
  */
/* USER CODE END Header_StartPasswordTask */
void StartPasswordTask(void *argument)
{
  /* USER CODE BEGIN 5 */
  (void)argument;
  printf("\r\nPassword Task start\r\n");

  Keypad_Init(&keypad);
  DoorHardware_Init();

  pin_entry_mode = PIN_MODE_NONE;
  card_enroll_active = false;
  memset(pending_new_pin, 0, sizeof(pending_new_pin));
  consecutive_auth_failures = 0U;
  last_capture_alert_tick = 0U;

  DoorUI_Init(&door_ui, NULL);
  if (SSD1306_Init(&holed, &hi2c1, SSD1306_DEFAULT_I2C_ADDRESS) == HAL_OK)
  {
    DoorUI_Init(&door_ui, &holed);
    DoorUI_ShowBootAnimation(&door_ui);
    DoorUI_ShowIdle(&door_ui);
  }
  else
  {
    printf("OLED not found\r\n");
  }

  /* Phase 1: 发送就绪消息 */
  printf("DOOR_LOCK_READY\r\n");

  /* Phase 3: 发送当前保存的密码和NFC卡信息（密码已哈希不输出明文） */
  printf("INFO:PASSWORD:HASHED\r\n");
  size_t count = AccessControl_GetCardCount(&access_control);
  for (size_t i = 0; i < count; i++)
  {
    const AccessControl_Card *card = AccessControl_GetCard(&access_control, i);
    if (card != NULL && card->uid_length > 0)
    {
      char uid_str[PN532_MAX_UID_LENGTH * 2 + 1];
      for (uint8_t j = 0; j < card->uid_length; j++)
      {
        sprintf(uid_str + j * 2, "%02X", card->uid[j]);
      }
      uid_str[card->uid_length * 2] = '\0';
      printf("INFO:NFC:%s:%s\r\n", uid_str, NfcRoleName(card->role));
    }
  }

  uint32_t last_heartbeat = HAL_GetTick();
  for (;;)
  {
    HandleKeypadKey(Keypad_GetKey(&keypad));

    /* Phase 1: 每 5 秒发送一次心跳 */
    uint32_t now = HAL_GetTick();
    if ((uint32_t)(now - last_heartbeat) >= 5000U)
    {
      last_heartbeat = now;
      printf("HEARTBEAT\r\n");
    }

    osDelay(10U);
  }
  /* USER CODE END 5 */
}

/* USER CODE BEGIN Header_StartNfcTask */
/**
* @brief Function implementing the NfcTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartNfcTask */
void StartNfcTask(void *argument)
{
  /* USER CODE BEGIN StartNfcTask */
  (void)argument;

  uint8_t last_uid[PN532_MAX_UID_LENGTH] = {0};
  uint8_t last_uid_len = 0U;
  bool card_present = false;

  printf("NFC Task start\r\n");
  if (PN532_Init(&hpn532, &hi2c1, PN532_DEFAULT_I2C_ADDRESS) != HAL_OK)
  {
    printf("[NFC] PN532 not found\r\n");
    for (;;)
    {
      osDelay(1000U);
    }
  }

  if (PN532_SAMConfig(&hpn532) != HAL_OK)
  {
    printf("[NFC] SAM config failed\r\n");
    for (;;)
    {
      osDelay(1000U);
    }
  }

  printf("[NFC] PN532 ready\r\n");

  for (;;)
  {
    uint8_t uid[PN532_MAX_UID_LENGTH] = {0};
    uint8_t uid_len = sizeof(uid);

    if (PN532_ReadCardUID(&hpn532, uid, &uid_len) != HAL_OK)
    {
      card_present = false;
      last_uid_len = 0U;
      osDelay(100U);
      continue;
    }

    bool same_card = card_present &&
                     uid_len == last_uid_len &&
                     memcmp(uid, last_uid, uid_len) == 0;

    if (!same_card)
    {
      memcpy(last_uid, uid, uid_len);
      last_uid_len = uid_len;
      card_present = true;

      if (card_enroll_active)
      {
        AccessControl_Role role = AccessControl_GetCardCount(&access_control) == 0U ?
                                  ACCESS_CONTROL_ROLE_ADMIN : ACCESS_CONTROL_ROLE_USER;
        AccessControl_Status status = AccessControl_AddCardWithRole(&access_control, uid, uid_len, role);
        AccessControl_Role saved_role = status == ACCESS_CONTROL_DUPLICATE ?
                                        AccessControl_GetRole(&access_control, uid, uid_len) : role;

        card_enroll_active = false;
        if (status == ACCESS_CONTROL_OK || status == ACCESS_CONTROL_DUPLICATE)
        {
          DoorUI_ShowMessage(&door_ui,
                             saved_role == ACCESS_CONTROL_ROLE_ADMIN ? "ADMIN SAVED" : "CARD SAVED",
                             saved_role == ACCESS_CONTROL_ROLE_ADMIN ? "ADMIN CARD" : "CAN OPEN DOOR",
                             NULL);
          StatusLed_Authorized();
          printf("[NFC] Card enrolled, role=%s, count=%lu\r\n",
                 NfcRoleName(saved_role),
                 (unsigned long)AccessControl_GetCardCount(&access_control));
        }
        else
        {
          DoorUI_ShowMessage(&door_ui, "CARD FAILED", "TRY AGAIN", NULL);
          DoorHardware_OnAccessDenied();
          printf("[NFC] Card enroll failed: %u\r\n", (unsigned int)status);
        }
        osDelay(700U);
        DoorUI_ShowIdle(&door_ui);
      }
      else
      {
        AccessControl_Role role = AccessControl_GetRole(&access_control, uid, uid_len);
        bool authorized = role == ACCESS_CONTROL_ROLE_ADMIN || role == ACCESS_CONTROL_ROLE_USER;

        printf("[NFC] Card detected, role=%s, authorized=%u\r\n", NfcRoleName(role), authorized ? 1U : 0U);
        (void)DoorEvent_Send(DOOR_EVENT_NFC_RESULT, authorized, uid, uid_len, NULL, role);
      }
    }

    osDelay(200U);
  }
  /* USER CODE END StartNfcTask */
}

/* USER CODE BEGIN Header_StartCtrlTask */
/**
* @brief Function implementing the CtrlTask thread.
* @param argument: Not used
* @retval None
*/
/* USER CODE END Header_StartCtrlTask */
void StartCtrlTask(void *argument)
{
  /* USER CODE BEGIN StartCtrlTask */
  (void)argument;

  DoorEvent event;

  for (;;)
  {
    if (osMessageQueueGet(DoorEventQueueHandle, &event, NULL, osWaitForever) == osOK &&
        (event.type == DOOR_EVENT_PIN_RESULT || event.type == DOOR_EVENT_NFC_RESULT))
    {
      HandleAuthResult(&event);
    }
  }
  /* USER CODE END StartCtrlTask */
}

/**
  * @brief  Period elapsed callback in non blocking mode
  * @note   This function is called  when TIM2 interrupt took place, inside
  * HAL_TIM_IRQHandler(). It makes a direct call to HAL_IncTick() to increment
  * a global variable "uwTick" used as application time base.
  * @param  htim : TIM handle
  * @retval None
  */
void HAL_TIM_PeriodElapsedCallback(TIM_HandleTypeDef *htim)
{
  /* USER CODE BEGIN Callback 0 */

  /* USER CODE END Callback 0 */
  if (htim->Instance == TIM2)
  {
    HAL_IncTick();
  }
  /* USER CODE BEGIN Callback 1 */

  /* USER CODE END Callback 1 */
}

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
