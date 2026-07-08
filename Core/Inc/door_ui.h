#ifndef DOOR_UI_H
#define DOOR_UI_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "ssd1306.h"

#define DOOR_UI_MAX_PIN_LENGTH 8U
#define DOOR_UI_TITLE_LENGTH 16U

typedef struct {
  SSD1306_HandleTypeDef *display;
  char pin[DOOR_UI_MAX_PIN_LENGTH + 1U];
  char pin_title[DOOR_UI_TITLE_LENGTH];
  uint8_t pin_length;
} DoorUI_HandleTypeDef;

void DoorUI_Init(DoorUI_HandleTypeDef *ui, SSD1306_HandleTypeDef *display);
void DoorUI_ShowBootAnimation(DoorUI_HandleTypeDef *ui);
void DoorUI_ShowIdle(DoorUI_HandleTypeDef *ui);
void DoorUI_ShowAccessResult(DoorUI_HandleTypeDef *ui, bool authorized);
void DoorUI_ShowMessage(DoorUI_HandleTypeDef *ui, const char *title, const char *line1, const char *line2);
void DoorUI_BeginPinEntry(DoorUI_HandleTypeDef *ui);
void DoorUI_BeginPinEntryWithTitle(DoorUI_HandleTypeDef *ui, const char *title);
bool DoorUI_EnterPinDigit(DoorUI_HandleTypeDef *ui, char digit);
bool DoorUI_BackspacePin(DoorUI_HandleTypeDef *ui);
const char *DoorUI_GetPin(const DoorUI_HandleTypeDef *ui);
uint8_t DoorUI_GetPinLength(const DoorUI_HandleTypeDef *ui);

#ifdef __cplusplus
}
#endif

#endif