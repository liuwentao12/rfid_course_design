#include "access_config.h"

#include <string.h>

static char authorized_pin[ACCESS_CONFIG_MAX_PIN_LENGTH + 1U] = "123456";

size_t AccessConfig_LoadAuthorizedCards(AccessControl *control)
{
  (void)control;
  return 0U;
}

bool AccessConfig_IsPinAuthorized(const char *pin)
{
  return pin != NULL && strcmp(pin, authorized_pin) == 0;
}

bool AccessConfig_SetPin(const char *pin)
{
  size_t length = pin == NULL ? 0U : strlen(pin);

  if (length == 0U || length > ACCESS_CONFIG_MAX_PIN_LENGTH)
  {
    return false;
  }

  memcpy(authorized_pin, pin, length + 1U);
  return true;
}
