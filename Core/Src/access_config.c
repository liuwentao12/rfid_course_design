#include "access_config.h"

#include <string.h>

static const char authorized_pin[] = "123456";

size_t AccessConfig_LoadAuthorizedCards(AccessControl *control)
{
  (void)control;
  return 0U;
}

bool AccessConfig_IsPinAuthorized(const char *pin)
{
  return pin != NULL && strcmp(pin, authorized_pin) == 0;
}