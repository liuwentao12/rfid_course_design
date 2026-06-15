#ifndef DOOR_UI_H
#define DOOR_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "ssd1306.h"

#define DOOR_UI_MAX_PIN_LENGTH 8U

typedef struct
{
  SSD1306_HandleTypeDef *display;
  char pin[DOOR_UI_MAX_PIN_LENGTH + 1U];
  uint8_t pin_length;
} DoorUI_HandleTypeDef;

void DoorUI_Init(DoorUI_HandleTypeDef *ui, SSD1306_HandleTypeDef *display);

/* 开机时显示一次简单进度动画。 */
void DoorUI_ShowBootAnimation(DoorUI_HandleTypeDef *ui);

/* 正常待机页面。 */
void DoorUI_ShowIdle(DoorUI_HandleTypeDef *ui);

/* 显示刷卡或密码验证结果。 */
void DoorUI_ShowAccessResult(DoorUI_HandleTypeDef *ui, bool authorized);

/* 进入密码输入模式并清空之前输入的密码。 */
void DoorUI_BeginPinEntry(DoorUI_HandleTypeDef *ui);

/*
 * 添加一个数字并刷新页面。
 * 页面只显示最新数字，之前输入的数字显示为 *。
 */
bool DoorUI_EnterPinDigit(DoorUI_HandleTypeDef *ui, char digit);

/* 删除最后一个数字并刷新页面。 */
bool DoorUI_BackspacePin(DoorUI_HandleTypeDef *ui);

const char *DoorUI_GetPin(const DoorUI_HandleTypeDef *ui);
uint8_t DoorUI_GetPinLength(const DoorUI_HandleTypeDef *ui);

#ifdef __cplusplus
}
#endif

#endif
