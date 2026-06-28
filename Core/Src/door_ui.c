#include "door_ui.h"

#include <string.h>

static void DoorUI_Refresh(DoorUI_HandleTypeDef *ui)
{
  if (ui != NULL && ui->display != NULL)
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
  char masked_pin[DOOR_UI_MAX_PIN_LENGTH + 1U];

  memset(masked_pin, 0, sizeof(masked_pin));
  for (uint8_t i = 0U; i < ui->pin_length; i++)
  {
    /*
     * 手机密码框的效果：
     * 最后输入的数字可见，之前输入的数字全部显示为星号。
     */
    masked_pin[i] = (i == (uint8_t)(ui->pin_length - 1U)) ? ui->pin[i] : '*';
  }

  SSD1306_Clear(ui->display);
  DoorUI_DrawHeader(ui, "ENTER PIN");
  SSD1306_DrawRect(ui->display, 2U, 14U, 124U, 16U, true);
  SSD1306_WriteString(ui->display, 7U, 18U, masked_pin, true);
  DoorUI_Refresh(ui);
}

void DoorUI_Init(DoorUI_HandleTypeDef *ui, SSD1306_HandleTypeDef *display)
{
  if (ui == NULL)
  {
    return;
  }

  memset(ui, 0, sizeof(*ui));
  ui->display = display;
}

void DoorUI_ShowBootAnimation(DoorUI_HandleTypeDef *ui)
{
  if (ui == NULL || ui->display == NULL)
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
  if (ui == NULL || ui->display == NULL)
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
  if (ui == NULL || ui->display == NULL)
  {
    return;
  }

  SSD1306_Clear(ui->display);
  DoorUI_DrawHeader(ui, authorized ? "ACCESS OK" : "ACCESS DENIED");

  if (authorized)
  {
    /* 一个简单的对勾。 */
    for (uint8_t i = 0U; i < 6U; i++)
    {
      SSD1306_DrawPixel(ui->display, (uint8_t)(48U + i), (uint8_t)(22U + i), true);
      SSD1306_DrawPixel(ui->display, (uint8_t)(53U + i), (uint8_t)(27U - i), true);
    }
  }
  else
  {
    /* 一个简单的叉号。 */
    for (uint8_t i = 0U; i < 12U; i++)
    {
      SSD1306_DrawPixel(ui->display, (uint8_t)(58U + i), (uint8_t)(16U + i), true);
      SSD1306_DrawPixel(ui->display, (uint8_t)(69U - i), (uint8_t)(16U + i), true);
    }
  }

  DoorUI_Refresh(ui);
}

//  清空密码，开始输入
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

//DoorUI_EnterPinDigit()
bool DoorUI_EnterPinDigit(DoorUI_HandleTypeDef *ui, char digit)
{
  if (ui == NULL || digit < '0' || digit > '9' || ui->pin_length >= DOOR_UI_MAX_PIN_LENGTH)
  {
    return false;
  }

  ui->pin[ui->pin_length] = digit;
  ui->pin_length++;
  ui->pin[ui->pin_length] = '\0';
  DoorUI_DrawPin(ui);

  return true;
}

//  删除最后一个数字
bool DoorUI_BackspacePin(DoorUI_HandleTypeDef *ui)
{
  if (ui == NULL || ui->pin_length == 0U)
  {
    return false;
  }

  ui->pin_length--;
  ui->pin[ui->pin_length] = '\0';
  DoorUI_DrawPin(ui);

  return true;
}

//  获取当前密码字符串
const char *DoorUI_GetPin(const DoorUI_HandleTypeDef *ui)
{
  return ui == NULL ? "" : ui->pin;
}

//获取当前密码长度
uint8_t DoorUI_GetPinLength(const DoorUI_HandleTypeDef *ui)
{
  return ui == NULL ? 0U : ui->pin_length;
}
