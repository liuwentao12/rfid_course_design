#ifndef ACCESS_CONTROL_H
#define ACCESS_CONTROL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#define ACCESS_CONTROL_MAX_CARDS 16U
#define ACCESS_CONTROL_MAX_UID_LENGTH 10U

typedef enum {
  ACCESS_CONTROL_OK = 0,
  ACCESS_CONTROL_INVALID_ARGUMENT,
  ACCESS_CONTROL_INVALID_UID_LENGTH,
  ACCESS_CONTROL_DUPLICATE,
  ACCESS_CONTROL_FULL,
  ACCESS_CONTROL_NOT_FOUND
} AccessControl_Status;

typedef struct {
  uint8_t uid[ACCESS_CONTROL_MAX_UID_LENGTH];
  uint8_t uid_length;
  bool enabled;
} AccessControl_Card;

typedef struct {
  AccessControl_Card cards[ACCESS_CONTROL_MAX_CARDS];
  size_t card_count;
} AccessControl;

void AccessControl_Init(AccessControl *control);
AccessControl_Status AccessControl_AddCard(AccessControl *control, const uint8_t *uid, uint8_t uid_length);
AccessControl_Status AccessControl_RemoveCard(AccessControl *control, const uint8_t *uid, uint8_t uid_length);
AccessControl_Status AccessControl_SetCardEnabled(AccessControl *control, const uint8_t *uid, uint8_t uid_length, bool enabled);
bool AccessControl_IsAuthorized(const AccessControl *control, const uint8_t *uid, uint8_t uid_length);
size_t AccessControl_GetCardCount(const AccessControl *control);
const AccessControl_Card *AccessControl_GetCard(const AccessControl *control, size_t index);

#ifdef __cplusplus
}
#endif

#endif