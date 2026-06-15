#ifndef SSD1306_H
#define SSD1306_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#include "stm32f1xx_hal.h"

#define SSD1306_WIDTH 128U
#define SSD1306_HEIGHT 32U
#define SSD1306_DEFAULT_I2C_ADDRESS (0x3CU << 1)

typedef struct
{
  I2C_HandleTypeDef *hi2c;
  uint16_t address;
  uint8_t buffer[SSD1306_WIDTH * SSD1306_HEIGHT / 8U];
} SSD1306_HandleTypeDef;

/* 初始化 SSD1306。成功后屏幕会被清空。 */
HAL_StatusTypeDef SSD1306_Init(SSD1306_HandleTypeDef *display,
                               I2C_HandleTypeDef *hi2c,
                               uint16_t address);

/* 修改显存后，调用此函数才会真正刷新到 OLED。 */
HAL_StatusTypeDef SSD1306_UpdateScreen(SSD1306_HandleTypeDef *display);

void SSD1306_Clear(SSD1306_HandleTypeDef *display);
void SSD1306_DrawPixel(SSD1306_HandleTypeDef *display, uint8_t x, uint8_t y, bool on);
void SSD1306_DrawRect(SSD1306_HandleTypeDef *display,
                      uint8_t x,
                      uint8_t y,
                      uint8_t width,
                      uint8_t height,
                      bool on);
void SSD1306_FillRect(SSD1306_HandleTypeDef *display,
                      uint8_t x,
                      uint8_t y,
                      uint8_t width,
                      uint8_t height,
                      bool on);

/*
 * 使用内置 5x7 ASCII 字体写文字。
 * x、y 是文字左上角坐标；字符实际占用 6x8 像素。
 */
void SSD1306_WriteString(SSD1306_HandleTypeDef *display,
                         uint8_t x,
                         uint8_t y,
                         const char *text,
                         bool on);

#ifdef __cplusplus
}
#endif

#endif
