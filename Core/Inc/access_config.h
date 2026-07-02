#ifndef ACCESS_CONFIG_H
#define ACCESS_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "access_control.h"

size_t AccessConfig_LoadAuthorizedCards(AccessControl *control);
bool AccessConfig_IsPinAuthorized(const char *pin);

#ifdef __cplusplus
}
#endif

#endif