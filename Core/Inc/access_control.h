#ifndef ACCESS_CONTROL_H
#define ACCESS_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

/*
 * 这个模块只负责管理“哪些 RFID 卡可以开门”。
 * 它不直接操作 PN532、GPIO 或门锁，便于在不同认证流程中复用。
 */

/* 最多保存 16 张卡；PN532 读取到的 UID 最长为 10 字节。 */
#define ACCESS_CONTROL_MAX_CARDS 16U
#define ACCESS_CONTROL_MAX_UID_LENGTH 10U

/* 调用添加、删除等函数后，通过这些状态值判断操作是否成功。 */
typedef enum
{
  ACCESS_CONTROL_OK = 0,             /* 操作成功。 */
  ACCESS_CONTROL_INVALID_ARGUMENT,   /* 传入了 NULL 等无效参数。 */
  ACCESS_CONTROL_INVALID_UID_LENGTH, /* UID 长度不是常见的 4、7 或 10 字节。 */
  ACCESS_CONTROL_DUPLICATE,          /* 这张卡已经在名单中。 */
  ACCESS_CONTROL_FULL,               /* 名单已保存 16 张卡，没有空位。 */
  ACCESS_CONTROL_NOT_FOUND           /* 名单中找不到这张卡。 */
} AccessControl_Status;

/* 名单中一张卡的数据。 */
typedef struct
{
  uint8_t uid[ACCESS_CONTROL_MAX_UID_LENGTH]; /* 卡片的唯一编号。 */
  uint8_t uid_length;                         /* 实际使用了 uid 数组中的几个字节。 */
  bool enabled;                               /* false 表示暂时禁止这张卡开门。 */
} AccessControl_Card;

/* 完整的授权名单。 */
typedef struct
{
  AccessControl_Card cards[ACCESS_CONTROL_MAX_CARDS]; /* 保存卡片的数组。 */
  size_t card_count;                                  /* 当前已经保存的卡片数量。 */
} AccessControl;

/*
 * 使用顺序：
 *
 * AccessControl control;
 * AccessControl_Init(&control);
 * AccessControl_AddCard(&control, uid, uid_length);
 *
 * if (AccessControl_IsAuthorized(&control, uid, uid_length)) {
 *   // 允许开门
 * }
 */

/* 清空并初始化授权名单，使用其他函数前必须先调用一次。 */
void AccessControl_Init(AccessControl *control);

/* 添加一张授权卡；添加成功后，这张卡默认可以开门。 */
AccessControl_Status AccessControl_AddCard(AccessControl *control,
                                           const uint8_t *uid,
                                           uint8_t uid_length);

/* 从名单中彻底删除一张卡。 */
AccessControl_Status AccessControl_RemoveCard(AccessControl *control,
                                              const uint8_t *uid,
                                              uint8_t uid_length);

/* 暂时启用或禁用一张卡，不需要把它从名单中删除。 */
AccessControl_Status AccessControl_SetCardEnabled(AccessControl *control,
                                                  const uint8_t *uid,
                                                  uint8_t uid_length,
                                                  bool enabled);

/* 查询一张卡当前是否有开门权限。 */
bool AccessControl_IsAuthorized(const AccessControl *control,
                                const uint8_t *uid,
                                uint8_t uid_length);

/* 获取名单中当前保存的卡片数量。 */
size_t AccessControl_GetCardCount(const AccessControl *control);

#ifdef __cplusplus
}
#endif

#endif
