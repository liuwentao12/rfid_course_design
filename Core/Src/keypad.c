#include "keypad.h"

#include <stdbool.h>

#include "main.h"

#define KEYPAD_ROW_COUNT 4U
#define KEYPAD_COLUMN_COUNT 4U
#define KEYPAD_DEBOUNCE_SCANS 3U

static GPIO_TypeDef *const keypad_column_ports[KEYPAD_COLUMN_COUNT] = {
  COL1_GPIO_Port, COL2_GPIO_Port, COL3_GPIO_Port, COL4_GPIO_Port
};

static const uint16_t keypad_column_pins[KEYPAD_COLUMN_COUNT] = {
  COL1_Pin, COL2_Pin, COL3_Pin, COL4_Pin
};

static GPIO_TypeDef *const keypad_row_ports[KEYPAD_ROW_COUNT] = {
  ROW1_GPIO_Port, ROW2_GPIO_Port, ROW3_GPIO_Port, ROW4_GPIO_Port
};

static const uint16_t keypad_row_pins[KEYPAD_ROW_COUNT] = {
  ROW1_Pin, ROW2_Pin, ROW3_Pin, ROW4_Pin
};

static const char keypad_map[KEYPAD_ROW_COUNT][KEYPAD_COLUMN_COUNT] = {
  {'1', '2', '3', 'A'},
  {'4', '5', '6', 'B'},
  {'7', '8', '9', 'C'},
  {'*', '0', '#', 'D'}
};

static void Keypad_SetAllColumns(GPIO_PinState state)
{
  for (uint8_t column = 0U; column < KEYPAD_COLUMN_COUNT; column++)
  {
    HAL_GPIO_WritePin(keypad_column_ports[column], keypad_column_pins[column], state);
  }
}

static char Keypad_ScanRaw(void)
{
  char detected_key = KEYPAD_NO_KEY;

  /*
   * 行引脚使用上拉输入。逐列输出低电平：
   * 如果某个按键按下，对应行会被该列拉低。
   */
  Keypad_SetAllColumns(GPIO_PIN_SET);

  for (uint8_t column = 0U; column < KEYPAD_COLUMN_COUNT; column++)
  {
    HAL_GPIO_WritePin(keypad_column_ports[column], keypad_column_pins[column], GPIO_PIN_RESET);

    for (uint8_t row = 0U; row < KEYPAD_ROW_COUNT; row++)
    {
      if (HAL_GPIO_ReadPin(keypad_row_ports[row], keypad_row_pins[row]) == GPIO_PIN_RESET)
      {
        detected_key = keypad_map[row][column];
        break;
      }
    }

    HAL_GPIO_WritePin(keypad_column_ports[column], keypad_column_pins[column], GPIO_PIN_SET);

    if (detected_key != KEYPAD_NO_KEY)
    {
      break;
    }
  }

  return detected_key;
}

void Keypad_Init(Keypad_HandleTypeDef *keypad)
{
  if (keypad == NULL)
  {
    return;
  }

  keypad->last_raw_key = KEYPAD_NO_KEY;
  keypad->stable_key = KEYPAD_NO_KEY;
  keypad->stable_count = 0U;
  keypad->key_reported = false;
  Keypad_SetAllColumns(GPIO_PIN_SET);
}

char Keypad_GetKey(Keypad_HandleTypeDef *keypad)
{
  char raw_key;

  if (keypad == NULL)
  {
    return KEYPAD_NO_KEY;
  }

  raw_key = Keypad_ScanRaw();

  if (raw_key != keypad->last_raw_key)
  {
    keypad->last_raw_key = raw_key;
    keypad->stable_count = 1U;
    return KEYPAD_NO_KEY;
  }

  if (keypad->stable_count < KEYPAD_DEBOUNCE_SCANS)
  {
    keypad->stable_count++;
  }

  if (keypad->stable_count < KEYPAD_DEBOUNCE_SCANS)
  {
    return KEYPAD_NO_KEY;
  }

  keypad->stable_key = raw_key;

  if (keypad->stable_key == KEYPAD_NO_KEY)
  {
    keypad->key_reported = false;
    return KEYPAD_NO_KEY;
  }

  if (keypad->key_reported)
  {
    return KEYPAD_NO_KEY;
  }

  keypad->key_reported = true;
  return keypad->stable_key;
}
