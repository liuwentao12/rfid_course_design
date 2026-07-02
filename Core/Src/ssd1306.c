#include "ssd1306.h"

#include <stddef.h>
#include <string.h>

#define SSD1306_CONTROL_COMMAND 0x00U
#define SSD1306_CONTROL_DATA 0x40U
#define SSD1306_FONT_FIRST_CHAR 32U
#define SSD1306_FONT_LAST_CHAR 90U
#define SSD1306_FONT_WIDTH 5U


static const uint8_t ssd1306_font[][SSD1306_FONT_WIDTH] = {
  {0x00U, 0x00U, 0x00U, 0x00U, 0x00U},
  {0x00U, 0x00U, 0x5FU, 0x00U, 0x00U},
  {0x00U, 0x07U, 0x00U, 0x07U, 0x00U},
  {0x14U, 0x7FU, 0x14U, 0x7FU, 0x14U},
  {0x24U, 0x2AU, 0x7FU, 0x2AU, 0x12U},
  {0x23U, 0x13U, 0x08U, 0x64U, 0x62U},
  {0x36U, 0x49U, 0x55U, 0x22U, 0x50U},
  {0x00U, 0x05U, 0x03U, 0x00U, 0x00U},
  {0x00U, 0x1CU, 0x22U, 0x41U, 0x00U},
  {0x00U, 0x41U, 0x22U, 0x1CU, 0x00U},
  {0x14U, 0x08U, 0x3EU, 0x08U, 0x14U},
  {0x08U, 0x08U, 0x3EU, 0x08U, 0x08U},
  {0x00U, 0x50U, 0x30U, 0x00U, 0x00U},
  {0x08U, 0x08U, 0x08U, 0x08U, 0x08U},
  {0x00U, 0x60U, 0x60U, 0x00U, 0x00U},
  {0x20U, 0x10U, 0x08U, 0x04U, 0x02U},
  {0x3EU, 0x51U, 0x49U, 0x45U, 0x3EU},
  {0x00U, 0x42U, 0x7FU, 0x40U, 0x00U},
  {0x42U, 0x61U, 0x51U, 0x49U, 0x46U},
  {0x21U, 0x41U, 0x45U, 0x4BU, 0x31U},
  {0x18U, 0x14U, 0x12U, 0x7FU, 0x10U},
  {0x27U, 0x45U, 0x45U, 0x45U, 0x39U},
  {0x3CU, 0x4AU, 0x49U, 0x49U, 0x30U},
  {0x01U, 0x71U, 0x09U, 0x05U, 0x03U},
  {0x36U, 0x49U, 0x49U, 0x49U, 0x36U},
  {0x06U, 0x49U, 0x49U, 0x29U, 0x1EU},
  {0x00U, 0x36U, 0x36U, 0x00U, 0x00U},
  {0x00U, 0x56U, 0x36U, 0x00U, 0x00U},
  {0x08U, 0x14U, 0x22U, 0x41U, 0x00U},
  {0x14U, 0x14U, 0x14U, 0x14U, 0x14U},
  {0x00U, 0x41U, 0x22U, 0x14U, 0x08U},
  {0x02U, 0x01U, 0x51U, 0x09U, 0x06U},
  {0x32U, 0x49U, 0x79U, 0x41U, 0x3EU},
  {0x7EU, 0x11U, 0x11U, 0x11U, 0x7EU},
  {0x7FU, 0x49U, 0x49U, 0x49U, 0x36U},
  {0x3EU, 0x41U, 0x41U, 0x41U, 0x22U},
  {0x7FU, 0x41U, 0x41U, 0x22U, 0x1CU},
  {0x7FU, 0x49U, 0x49U, 0x49U, 0x41U},
  {0x7FU, 0x09U, 0x09U, 0x09U, 0x01U},
  {0x3EU, 0x41U, 0x49U, 0x49U, 0x7AU},
  {0x7FU, 0x08U, 0x08U, 0x08U, 0x7FU},
  {0x00U, 0x41U, 0x7FU, 0x41U, 0x00U},
  {0x20U, 0x40U, 0x41U, 0x3FU, 0x01U},
  {0x7FU, 0x08U, 0x14U, 0x22U, 0x41U},
  {0x7FU, 0x40U, 0x40U, 0x40U, 0x40U},
  {0x7FU, 0x02U, 0x0CU, 0x02U, 0x7FU},
  {0x7FU, 0x04U, 0x08U, 0x10U, 0x7FU},
  {0x3EU, 0x41U, 0x41U, 0x41U, 0x3EU},
  {0x7FU, 0x09U, 0x09U, 0x09U, 0x06U},
  {0x3EU, 0x41U, 0x51U, 0x21U, 0x5EU},
  {0x7FU, 0x09U, 0x19U, 0x29U, 0x46U},
  {0x46U, 0x49U, 0x49U, 0x49U, 0x31U},
  {0x01U, 0x01U, 0x7FU, 0x01U, 0x01U},
  {0x3FU, 0x40U, 0x40U, 0x40U, 0x3FU},
  {0x1FU, 0x20U, 0x40U, 0x20U, 0x1FU},
  {0x7FU, 0x20U, 0x18U, 0x20U, 0x7FU},
  {0x63U, 0x14U, 0x08U, 0x14U, 0x63U},
  {0x03U, 0x04U, 0x78U, 0x04U, 0x03U},
  {0x61U, 0x51U, 0x49U, 0x45U, 0x43U}
};

static HAL_StatusTypeDef SSD1306_WriteCommand(SSD1306_HandleTypeDef *display, uint8_t command)
{
  uint8_t packet[] = {SSD1306_CONTROL_COMMAND, command};

  return HAL_I2C_Master_Transmit(display->hi2c,
                                 display->address,
                                 packet,
                                 sizeof(packet),
                                 100U);
}

static HAL_StatusTypeDef SSD1306_WriteCommands(SSD1306_HandleTypeDef *display,
                                               const uint8_t *commands,
                                               size_t command_count)
{
  for (size_t i = 0U; i < command_count; i++)
  {
    if (SSD1306_WriteCommand(display, commands[i]) != HAL_OK)
    {
      return HAL_ERROR;
    }
  }

  return HAL_OK;
}

HAL_StatusTypeDef SSD1306_Init(SSD1306_HandleTypeDef *display,
                               I2C_HandleTypeDef *hi2c,
                               uint16_t address)
{
  static const uint8_t init_commands[] = {
    0xAEU,
    0xD5U, 0x80U,
    0xA8U, 0x1FU,
    0xD3U, 0x00U,
    0x40U,
    0x8DU, 0x14U,
    0x20U, 0x02U,
    0xA1U,
    0xC8U,
    0xDAU, 0x02U,
    0x81U, 0x8FU,
    0xD9U, 0xF1U,
    0xDBU, 0x40U,
    0xA4U,
    0xA6U,
    0xAFU
  };

  if (display == NULL || hi2c == NULL)
  {
    return HAL_ERROR;
  }

  display->hi2c = hi2c;
  display->address = address;

  if (HAL_I2C_IsDeviceReady(hi2c, address, 2U, 20U) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (SSD1306_WriteCommands(display, init_commands, sizeof(init_commands)) != HAL_OK)
  {
    return HAL_ERROR;
  }

  SSD1306_Clear(display);
  return SSD1306_UpdateScreen(display);
}

HAL_StatusTypeDef SSD1306_UpdateScreen(SSD1306_HandleTypeDef *display)
{
  uint8_t packet[SSD1306_WIDTH + 1U];

  if (display == NULL || display->hi2c == NULL)
  {
    return HAL_ERROR;
  }

  packet[0] = SSD1306_CONTROL_DATA;

  for (uint8_t page = 0U; page < (SSD1306_HEIGHT / 8U); page++)
  {
    if (SSD1306_WriteCommand(display, (uint8_t)(0xB0U + page)) != HAL_OK ||
        SSD1306_WriteCommand(display, 0x00U) != HAL_OK ||
        SSD1306_WriteCommand(display, 0x10U) != HAL_OK)
    {
      return HAL_ERROR;
    }

    memcpy(&packet[1], &display->buffer[page * SSD1306_WIDTH], SSD1306_WIDTH);
    if (HAL_I2C_Master_Transmit(display->hi2c,
                                display->address,
                                packet,
                                sizeof(packet),
                                200U) != HAL_OK)
    {
      return HAL_ERROR;
    }
  }

  return HAL_OK;
}

void SSD1306_Clear(SSD1306_HandleTypeDef *display)
{
  if (display != NULL)
  {
    memset(display->buffer, 0, sizeof(display->buffer));
  }
}

void SSD1306_DrawPixel(SSD1306_HandleTypeDef *display, uint8_t x, uint8_t y, bool on)
{
  uint16_t index;
  uint8_t mask;

  if (display == NULL || x >= SSD1306_WIDTH || y >= SSD1306_HEIGHT)
  {
    return;
  }

  index = x + ((uint16_t)(y / 8U) * SSD1306_WIDTH);
  mask = (uint8_t)(1U << (y % 8U));

  if (on)
  {
    display->buffer[index] |= mask;
  }
  else
  {
    display->buffer[index] &= (uint8_t)~mask;
  }
}

void SSD1306_DrawRect(SSD1306_HandleTypeDef *display,
                      uint8_t x,
                      uint8_t y,
                      uint8_t width,
                      uint8_t height,
                      bool on)
{
  if (width == 0U || height == 0U)
  {
    return;
  }

  for (uint8_t offset = 0U; offset < width; offset++)
  {
    SSD1306_DrawPixel(display, (uint8_t)(x + offset), y, on);
    SSD1306_DrawPixel(display, (uint8_t)(x + offset), (uint8_t)(y + height - 1U), on);
  }

  for (uint8_t offset = 0U; offset < height; offset++)
  {
    SSD1306_DrawPixel(display, x, (uint8_t)(y + offset), on);
    SSD1306_DrawPixel(display, (uint8_t)(x + width - 1U), (uint8_t)(y + offset), on);
  }
}

void SSD1306_FillRect(SSD1306_HandleTypeDef *display,
                      uint8_t x,
                      uint8_t y,
                      uint8_t width,
                      uint8_t height,
                      bool on)
{
  for (uint8_t row = 0U; row < height; row++)
  {
    for (uint8_t column = 0U; column < width; column++)
    {
      SSD1306_DrawPixel(display, (uint8_t)(x + column), (uint8_t)(y + row), on);
    }
  }
}

static void SSD1306_WriteChar(SSD1306_HandleTypeDef *display,
                              uint8_t x,
                              uint8_t y,
                              char character,
                              bool on)
{
  uint8_t code = (uint8_t)character;

  if (code >= (uint8_t)'a' && code <= (uint8_t)'z')
  {
    code = (uint8_t)(code - (uint8_t)'a' + (uint8_t)'A');
  }

  if (code < SSD1306_FONT_FIRST_CHAR || code > SSD1306_FONT_LAST_CHAR)
  {
    code = (uint8_t)'?';
  }

  const uint8_t *glyph = ssd1306_font[code - SSD1306_FONT_FIRST_CHAR];
  for (uint8_t column = 0U; column < SSD1306_FONT_WIDTH; column++)
  {
    for (uint8_t row = 0U; row < 7U; row++)
    {
      bool pixel = (glyph[column] & (uint8_t)(1U << row)) != 0U;
      SSD1306_DrawPixel(display, (uint8_t)(x + column), (uint8_t)(y + row), pixel && on);
    }
  }
}

void SSD1306_WriteString(SSD1306_HandleTypeDef *display,
                         uint8_t x,
                         uint8_t y,
                         const char *text,
                         bool on)
{
  if (display == NULL || text == NULL)
  {
    return;
  }

  while (*text != '\0' && x <= (SSD1306_WIDTH - 6U))
  {
    SSD1306_WriteChar(display, x, y, *text, on);
    x = (uint8_t)(x + 6U);
    text++;
  }
}
