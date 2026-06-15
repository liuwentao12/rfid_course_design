#include "access_config.h"

#include <string.h>

/* 学习阶段先把密码固定在程序中；之后可以改为从 Flash 读取。 */
static const char authorized_pin[] = "123456";

size_t AccessConfig_LoadAuthorizedCards(AccessControl *control)
{
  size_t loaded_count = 0U;

  /*
   * 正式模式的授权卡配置位置。
   *
   * 1. 从串口复制 PN532 打印出的 UID。
   * 2. 按照下面示例创建数组。
   * 3. 调用 AccessControl_AddCard() 把它加入授权名单。
   *
   * 示例：串口打印 "Card UID: DE AD BE EF"
   *
   * const uint8_t owner_card[] = {0xDEU, 0xADU, 0xBEU, 0xEFU};
   * if (AccessControl_AddCard(control, owner_card, sizeof(owner_card)) == ACCESS_CONTROL_OK) {
   *   loaded_count++;
   * }
   *
   * 可以按照相同格式继续添加其他卡。
   * 当前没有填写任何 UID，因此固件会拒绝所有卡。
   */

  return loaded_count;
}

bool AccessConfig_IsPinAuthorized(const char *pin)
{
  return pin != NULL && strcmp(pin, authorized_pin) == 0;
}
