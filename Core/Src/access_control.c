#include "access_control.h"

#include <string.h>

static bool AccessControl_IsUidLengthValid(uint8_t length)
{
  return length == 4U || length == 7U || length == 10U;
}

static bool AccessControl_IsRoleValid(AccessControl_Role role)
{
  return role == ACCESS_CONTROL_ROLE_USER || role == ACCESS_CONTROL_ROLE_ADMIN;
}

static size_t AccessControl_FindCard(const AccessControl *control, const uint8_t *uid, uint8_t uid_length)
{
  if (control == NULL || uid == NULL || !AccessControl_IsUidLengthValid(uid_length))
  {
    return ACCESS_CONTROL_MAX_CARDS;
  }

  for (size_t i = 0U; i < control->card_count; i++)
  {
    const AccessControl_Card *card = &control->cards[i];
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
    memset(control, 0, sizeof(*control));
  }
}

AccessControl_Status AccessControl_AddCard(AccessControl *control, const uint8_t *uid, uint8_t uid_length)
{
  return AccessControl_AddCardWithRole(control, uid, uid_length, ACCESS_CONTROL_ROLE_USER);
}

AccessControl_Status AccessControl_AddCardWithRole(AccessControl *control, const uint8_t *uid, uint8_t uid_length, AccessControl_Role role)
{
  if (control == NULL || uid == NULL)
  {
    return ACCESS_CONTROL_INVALID_ARGUMENT;
  }
  if (!AccessControl_IsUidLengthValid(uid_length))
  {
    return ACCESS_CONTROL_INVALID_UID_LENGTH;
  }
  if (!AccessControl_IsRoleValid(role))
  {
    return ACCESS_CONTROL_INVALID_ARGUMENT;
  }
  if (AccessControl_FindCard(control, uid, uid_length) != ACCESS_CONTROL_MAX_CARDS)
  {
    return ACCESS_CONTROL_DUPLICATE;
  }
  if (control->card_count >= ACCESS_CONTROL_MAX_CARDS)
  {
    return ACCESS_CONTROL_FULL;
  }

  AccessControl_Card *card = &control->cards[control->card_count++];
  memcpy(card->uid, uid, uid_length);
  card->uid_length = uid_length;
  card->role = role;
  card->enabled = true;
  return ACCESS_CONTROL_OK;
}

AccessControl_Status AccessControl_RemoveCard(AccessControl *control, const uint8_t *uid, uint8_t uid_length)
{
  if (control == NULL || uid == NULL)
  {
    return ACCESS_CONTROL_INVALID_ARGUMENT;
  }
  if (!AccessControl_IsUidLengthValid(uid_length))
  {
    return ACCESS_CONTROL_INVALID_UID_LENGTH;
  }

  size_t index = AccessControl_FindCard(control, uid, uid_length);
  if (index == ACCESS_CONTROL_MAX_CARDS)
  {
    return ACCESS_CONTROL_NOT_FOUND;
  }

  control->card_count--;
  if (index < control->card_count)
  {
    control->cards[index] = control->cards[control->card_count];
  }
  memset(&control->cards[control->card_count], 0, sizeof(control->cards[0]));
  return ACCESS_CONTROL_OK;
}

AccessControl_Status AccessControl_SetCardEnabled(AccessControl *control, const uint8_t *uid, uint8_t uid_length, bool enabled)
{
  if (control == NULL || uid == NULL)
  {
    return ACCESS_CONTROL_INVALID_ARGUMENT;
  }
  if (!AccessControl_IsUidLengthValid(uid_length))
  {
    return ACCESS_CONTROL_INVALID_UID_LENGTH;
  }

  size_t index = AccessControl_FindCard(control, uid, uid_length);
  if (index == ACCESS_CONTROL_MAX_CARDS)
  {
    return ACCESS_CONTROL_NOT_FOUND;
  }

  control->cards[index].enabled = enabled;
  return ACCESS_CONTROL_OK;
}

bool AccessControl_IsAuthorized(const AccessControl *control, const uint8_t *uid, uint8_t uid_length)
{
  AccessControl_Role role = AccessControl_GetRole(control, uid, uid_length);
  return role == ACCESS_CONTROL_ROLE_USER || role == ACCESS_CONTROL_ROLE_ADMIN;
}

AccessControl_Role AccessControl_GetRole(const AccessControl *control, const uint8_t *uid, uint8_t uid_length)
{
  size_t index = AccessControl_FindCard(control, uid, uid_length);
  if (index == ACCESS_CONTROL_MAX_CARDS || !control->cards[index].enabled)
  {
    return ACCESS_CONTROL_ROLE_NONE;
  }
  return control->cards[index].role;
}

size_t AccessControl_GetCardCount(const AccessControl *control)
{
  return control == NULL ? 0U : control->card_count;
}

const AccessControl_Card *AccessControl_GetCard(const AccessControl *control, size_t index)
{
  if (control == NULL || index >= control->card_count)
  {
    return NULL;
  }
  return &control->cards[index];
}