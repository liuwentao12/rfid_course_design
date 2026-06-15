#include "access_control.h"

#include <assert.h>
#include <stdio.h>

/*
 * 测试文件也可以当成这个模块的调用示例来读。
 *
 * assert(条件) 表示“这里的条件必须成立”：
 * 条件成立时继续运行，条件不成立时测试立即失败。
 */

/* 示例一：添加卡片，然后检查它能否通过授权。 */
static void TestAddAndAuthorize(void)
{
  AccessControl control;

  /* 模拟 PN532 读取到的一张 4 字节 UID 卡。 */
  const uint8_t uid[] = {0xDEU, 0xADU, 0xBEU, 0xEFU};

  /* 使用授权名单前，先初始化。 */
  AccessControl_Init(&control);

  /* 新名单应当为空，并且这张卡还不能开门。 */
  assert(AccessControl_GetCardCount(&control) == 0U);
  assert(!AccessControl_IsAuthorized(&control, uid, sizeof(uid)));

  /* 添加后，名单数量变为 1，这张卡可以开门。 */
  assert(AccessControl_AddCard(&control, uid, sizeof(uid)) == ACCESS_CONTROL_OK);
  assert(AccessControl_GetCardCount(&control) == 1U);
  assert(AccessControl_IsAuthorized(&control, uid, sizeof(uid)));

  /* 再次添加同一个 UID，应当报告重复。 */
  assert(AccessControl_AddCard(&control, uid, sizeof(uid)) == ACCESS_CONTROL_DUPLICATE);
}

/* 示例二：暂时禁用、重新启用和彻底删除卡片。 */
static void TestDisableAndRemove(void)
{
  AccessControl control;
  const uint8_t uid[] = {1U, 2U, 3U, 4U, 5U, 6U, 7U};

  AccessControl_Init(&control);
  assert(AccessControl_AddCard(&control, uid, sizeof(uid)) == ACCESS_CONTROL_OK);
  assert(AccessControl_SetCardEnabled(&control, uid, sizeof(uid), false) == ACCESS_CONTROL_OK);
  assert(!AccessControl_IsAuthorized(&control, uid, sizeof(uid)));
  assert(AccessControl_SetCardEnabled(&control, uid, sizeof(uid), true) == ACCESS_CONTROL_OK);
  assert(AccessControl_IsAuthorized(&control, uid, sizeof(uid)));
  assert(AccessControl_RemoveCard(&control, uid, sizeof(uid)) == ACCESS_CONTROL_OK);
  assert(AccessControl_GetCardCount(&control) == 0U);
  assert(!AccessControl_IsAuthorized(&control, uid, sizeof(uid)));
  assert(AccessControl_RemoveCard(&control, uid, sizeof(uid)) == ACCESS_CONTROL_NOT_FOUND);
}

/* 示例三：检查错误参数和名单容量上限。 */
static void TestValidationAndCapacity(void)
{
  AccessControl control;
  uint8_t uid[4] = {0U};

  AccessControl_Init(&control);
  assert(AccessControl_AddCard(NULL, uid, sizeof(uid)) == ACCESS_CONTROL_INVALID_ARGUMENT);
  assert(AccessControl_AddCard(&control, NULL, sizeof(uid)) == ACCESS_CONTROL_INVALID_ARGUMENT);
  assert(AccessControl_AddCard(&control, uid, 5U) == ACCESS_CONTROL_INVALID_UID_LENGTH);

  /* 修改 UID 的第一个字节，模拟添加 16 张不同的卡。 */
  for (size_t i = 0U; i < ACCESS_CONTROL_MAX_CARDS; i++)
  {
    uid[0] = (uint8_t)i;
    assert(AccessControl_AddCard(&control, uid, sizeof(uid)) == ACCESS_CONTROL_OK);
  }

  uid[0] = 0xFFU;
  assert(AccessControl_AddCard(&control, uid, sizeof(uid)) == ACCESS_CONTROL_FULL);
}

/* 示例四：开头相同但长度不同的 UID，不应该被误认成同一张卡。 */
static void TestUidLengthsDoNotAlias(void)
{
  AccessControl control;
  const uint8_t short_uid[] = {1U, 2U, 3U, 4U};
  const uint8_t long_uid[] = {1U, 2U, 3U, 4U, 5U, 6U, 7U};

  AccessControl_Init(&control);
  assert(AccessControl_AddCard(&control, short_uid, sizeof(short_uid)) == ACCESS_CONTROL_OK);
  assert(!AccessControl_IsAuthorized(&control, long_uid, sizeof(long_uid)));
}

int main(void)
{
  /* 依次运行上面的四组测试。 */
  TestAddAndAuthorize();
  TestDisableAndRemove();
  TestValidationAndCapacity();
  TestUidLengthsDoNotAlias();

  puts("access_control tests passed");
  return 0;
}
