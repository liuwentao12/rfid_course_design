#ifndef KEYPAD_H
#define KEYPAD_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "stm32f1xx_hal.h"

#define KEYPAD_NO_KEY '\0'

#define KEYPAD_KEY_CONFIRM 'O'
#define KEYPAD_KEY_DELETE  'X'
#define KEYPAD_KEY_CHANGE  'M'
#define KEYPAD_KEY_BACK    'R'
#define KEYPAD_KEY_UNLOCK  'U'
#define KEYPAD_KEY_ADMIN   'A'

typedef struct
{
  char last_raw_key;
  char stable_key;
  uint8_t stable_count;
  bool key_reported;
} Keypad_HandleTypeDef;

/*
 * 初始化按键扫描状态。
 * GPIO 引脚本身仍由 CubeMX 生成的 MX_GPIO_Init() 配置。
 */
void Keypad_Init(Keypad_HandleTypeDef *keypad);

/*
 * 扫描一次键盘并执行消抖。
 *
 * 建议每 10 至 20 ms 调用一次。
 * 每次完整按下只返回一个字符；没有新按键时返回 KEYPAD_NO_KEY。
 */
char Keypad_GetKey(Keypad_HandleTypeDef *keypad);

#ifdef __cplusplus
}
#endif

#endif
