#include "door_ui.h"

#include <string.h>

static bool DoorUI_IsReady(const DoorUI_HandleTypeDef *ui)
{
  return ui != NULL && ui->display != NULL;
}

static void DoorUI_Refresh(DoorUI_HandleTypeDef *ui)
{
  if (DoorUI_IsReady(ui))
  {
    (void)SSD1306_UpdateScreen(ui->display);
  }
}

static void DoorUI_DrawHeader(DoorUI_HandleTypeDef *ui, const char *title)
{
  SSD1306_WriteString(ui->display, 2U, 1U, title, true);
  SSD1306_FillRect(ui->display, 0U, 9U, SSD1306_WIDTH, 1U, true);
}

static void DoorUI_DrawPin(DoorUI_HandleTypeDef *ui)
{
  char masked_pin[DOOR_UI_MAX_PIN_LENGTH + 1U] = {0};

  if (!DoorUI_IsReady(ui))
  {
    return;
  }

  memset(masked_pin, '*', ui->pin_length);
  if (ui->pin_length > 0U)
  {
    masked_pin[ui->pin_length - 1U] = ui->pin[ui->pin_length - 1U];
  }

  SSD1306_Clear(ui->display);
  DoorUI_DrawHeader(ui, "ENTER PIN");
  SSD1306_DrawRect(ui->display, 2U, 14U, 124U, 16U, true);
  SSD1306_WriteString(ui->display, 7U, 18U, masked_pin, true);
  DoorUI_Refresh(ui);
}

void DoorUI_Init(DoorUI_HandleTypeDef *ui, SSD1306_HandleTypeDef *display)
{
  if (ui != NULL)
  {
    memset(ui, 0, sizeof(*ui));
    ui->display = display;
  }
}

void DoorUI_ShowBootAnimation(DoorUI_HandleTypeDef *ui)
{
  if (!DoorUI_IsReady(ui))
  {
    return;
  }

  for (uint8_t progress = 0U; progress <= 100U; progress += 10U)
  {
    SSD1306_Clear(ui->display);
    SSD1306_WriteString(ui->display, 34U, 3U, "DOOR LOCK", true);
    SSD1306_DrawRect(ui->display, 8U, 18U, 112U, 10U, true);
    SSD1306_FillRect(ui->display, 11U, 21U, progress, 4U, true);
    DoorUI_Refresh(ui);
    HAL_Delay(45U);
  }
}

void DoorUI_ShowIdle(DoorUI_HandleTypeDef *ui)
{
  if (!DoorUI_IsReady(ui))
  {
    return;
  }

  SSD1306_Clear(ui->display);
  DoorUI_DrawHeader(ui, "DOOR LOCK");
  SSD1306_WriteString(ui->display, 13U, 15U, "SCAN CARD OR PIN", true);
  SSD1306_WriteString(ui->display, 46U, 24U, "READY", true);
  DoorUI_Refresh(ui);
}

void DoorUI_ShowAccessResult(DoorUI_HandleTypeDef *ui, bool authorized)
{
  if (!DoorUI_IsReady(ui))
  {
    return;
  }

  SSD1306_Clear(ui->display);
  DoorUI_DrawHeader(ui, authorized ? "ACCESS OK" : "ACCESS DENIED");

  for (uint8_t i = 0U; i < (authorized ? 6U : 12U); i++)
  {
    if (authorized)
    {
      SSD1306_DrawPixel(ui->display, (uint8_t)(48U + i), (uint8_t)(22U + i), true);
      SSD1306_DrawPixel(ui->display, (uint8_t)(53U + i), (uint8_t)(27U - i), true);
    }
    else
    {
      SSD1306_DrawPixel(ui->display, (uint8_t)(58U + i), (uint8_t)(16U + i), true);
      SSD1306_DrawPixel(ui->display, (uint8_t)(69U - i), (uint8_t)(16U + i), true);
    }
  }

  DoorUI_Refresh(ui);
}

void DoorUI_BeginPinEntry(DoorUI_HandleTypeDef *ui)
{
  if (ui == NULL)
  {
    return;
  }

  memset(ui->pin, 0, sizeof(ui->pin));
  ui->pin_length = 0U;
  DoorUI_DrawPin(ui);
}

bool DoorUI_EnterPinDigit(DoorUI_HandleTypeDef *ui, char digit)
{
  if (ui == NULL || digit < '0' || digit > '9' || ui->pin_length >= DOOR_UI_MAX_PIN_LENGTH)
  {
    return false;
  }

  ui->pin[ui->pin_length++] = digit;
  ui->pin[ui->pin_length] = '\0';
  DoorUI_DrawPin(ui);
  return true;
}

bool DoorUI_BackspacePin(DoorUI_HandleTypeDef *ui)
{
  if (ui == NULL || ui->pin_length == 0U)
  {
    return false;
  }

  ui->pin[--ui->pin_length] = '\0';
  DoorUI_DrawPin(ui);
  return true;
}

const char *DoorUI_GetPin(const DoorUI_HandleTypeDef *ui)
{
  return ui == NULL ? "" : ui->pin;
}

uint8_t DoorUI_GetPinLength(const DoorUI_HandleTypeDef *ui)
{
  return ui == NULL ? 0U : ui->pin_length;
}