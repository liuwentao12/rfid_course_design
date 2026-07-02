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

typedef struct {
  char last_raw_key;
  char stable_key;
  uint8_t stable_count;
  bool key_reported;
} Keypad_HandleTypeDef;

void Keypad_Init(Keypad_HandleTypeDef *keypad);
char Keypad_GetKey(Keypad_HandleTypeDef *keypad);

#ifdef __cplusplus
}
#endif

#endif