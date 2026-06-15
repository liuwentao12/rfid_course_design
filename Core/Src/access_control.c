#include "access_control.h"

#include <string.h>

/*
 * RFID 卡的 UID 通常是 4、7 或 10 字节。
 * static 表示这个辅助函数只给当前 .c 文件内部使用。
 */
static bool AccessControl_IsUidLengthValid(uint8_t uid_length)
{
  return uid_length == 4U || uid_length == 7U || uid_length == 10U;
}

/*
 * 在授权名单中查找指定 UID。
 *
 * 找到时：返回这张卡在 cards 数组中的下标。
 * 找不到时：返回 ACCESS_CONTROL_MAX_CARDS。
 *
 * 因为有效下标只有 0 到 ACCESS_CONTROL_MAX_CARDS - 1，
 * 所以 ACCESS_CONTROL_MAX_CARDS 可以安全地表示“没有找到”。
 */
static size_t AccessControl_FindCard(const AccessControl *control,
                                     const uint8_t *uid,
                                     uint8_t uid_length)
{
  /* 先检查参数，避免访问无效地址或比较错误长度的 UID。 */
  if (control == NULL || uid == NULL || !AccessControl_IsUidLengthValid(uid_length))
  {
    return ACCESS_CONTROL_MAX_CARDS;
  }

  /* 从名单中的第一张卡开始，逐张比较。 */
  for (size_t i = 0U; i < control->card_count; i++)
  {
    const AccessControl_Card *card = &control->cards[i];

    /*
     * UID 长度和每个字节都相同时，才认为是同一张卡。
     * memcmp(...) == 0 表示两段内存中的数据完全相同。
     */
    if (card->uid_length == uid_length && memcmp(card->uid, uid, uid_length) == 0)
    {
      return i;
    }
  }

  return ACCESS_CONTROL_MAX_CARDS;
}

void AccessControl_Init(AccessControl *control)
{
  if (control != NULL)
  {
    /* 把整个结构体清零，得到一个 card_count 为 0 的空名单。 */
    memset(control, 0, sizeof(*control));
  }
}

AccessControl_Status AccessControl_AddCard(AccessControl *control,
                                           const uint8_t *uid,
                                           uint8_t uid_length)
{
  AccessControl_Card *card;

  /* 指针为 NULL 时不能继续使用，否则程序可能崩溃。 */
  if (control == NULL || uid == NULL)
  {
    return ACCESS_CONTROL_INVALID_ARGUMENT;
  }

  if (!AccessControl_IsUidLengthValid(uid_length))
  {
    return ACCESS_CONTROL_INVALID_UID_LENGTH;
  }

  /* 不允许把同一张卡重复加入名单。 */
  if (AccessControl_FindCard(control, uid, uid_length) != ACCESS_CONTROL_MAX_CARDS)
  {
    return ACCESS_CONTROL_DUPLICATE;
  }

  /* 数组只有固定数量的位置，满了以后不能继续添加。 */
  if (control->card_count >= ACCESS_CONTROL_MAX_CARDS)
  {
    return ACCESS_CONTROL_FULL;
  }

  /* card_count 同时也是下一个空位置的下标。 */
  card = &control->cards[control->card_count];

  /* 把传入的 UID 复制到名单中，并记录长度和启用状态。 */
  memcpy(card->uid, uid, uid_length);
  card->uid_length = uid_length;
  card->enabled = true;
  control->card_count++;

  return ACCESS_CONTROL_OK;
}

AccessControl_Status AccessControl_RemoveCard(AccessControl *control,
                                              const uint8_t *uid,
                                              uint8_t uid_length)
{
  size_t index;

  if (control == NULL || uid == NULL)
  {
    return ACCESS_CONTROL_INVALID_ARGUMENT;
  }

  if (!AccessControl_IsUidLengthValid(uid_length))
  {
    return ACCESS_CONTROL_INVALID_UID_LENGTH;
  }

  index = AccessControl_FindCard(control, uid, uid_length);
  if (index == ACCESS_CONTROL_MAX_CARDS)
  {
    return ACCESS_CONTROL_NOT_FOUND;
  }

  /*
   * 删除后会留下一个空洞。这里把最后一张卡搬到空洞中，
   * 不需要移动后面的所有卡，执行时间更短。
   */
  control->card_count--;
  if (index < control->card_count)
  {
    control->cards[index] = control->cards[control->card_count];
  }

  /* 清空原来的最后一个位置，避免残留旧数据。 */
  memset(&control->cards[control->card_count], 0, sizeof(control->cards[0]));

  return ACCESS_CONTROL_OK;
}

AccessControl_Status AccessControl_SetCardEnabled(AccessControl *control,
                                                  const uint8_t *uid,
                                                  uint8_t uid_length,
                                                  bool enabled)
{
  size_t index;

  if (control == NULL || uid == NULL)
  {
    return ACCESS_CONTROL_INVALID_ARGUMENT;
  }

  if (!AccessControl_IsUidLengthValid(uid_length))
  {
    return ACCESS_CONTROL_INVALID_UID_LENGTH;
  }

  index = AccessControl_FindCard(control, uid, uid_length);
  if (index == ACCESS_CONTROL_MAX_CARDS)
  {
    return ACCESS_CONTROL_NOT_FOUND;
  }

  /* enabled 为 true 时允许开门，false 时暂时拒绝。 */
  control->cards[index].enabled = enabled;
  return ACCESS_CONTROL_OK;
}

bool AccessControl_IsAuthorized(const AccessControl *control,
                                const uint8_t *uid,
                                uint8_t uid_length)
{
  size_t index = AccessControl_FindCard(control, uid, uid_length);

  /* 必须同时满足“名单中存在”和“没有被禁用”才算授权。 */
  return index != ACCESS_CONTROL_MAX_CARDS && control->cards[index].enabled;
}

size_t AccessControl_GetCardCount(const AccessControl *control)
{
  /* 三目运算符：control 为 NULL 时返回 0，否则返回 card_count。 */
  return control == NULL ? 0U : control->card_count;
}
