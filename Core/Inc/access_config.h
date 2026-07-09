#ifndef ACCESS_CONFIG_H
#define ACCESS_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>

#include "access_control.h"
#define ACCESS_CONFIG_MAX_PIN_LENGTH 8U


size_t AccessConfig_LoadAuthorizedCards(AccessControl *control);
const char *AccessConfig_GetPin(void);
bool AccessConfig_SetPin(const char *pin);
bool AccessConfig_IsPinAuthorized(const char *pin);

#ifdef __cplusplus
}
#endif

#endif