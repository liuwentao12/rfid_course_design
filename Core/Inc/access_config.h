#ifndef ACCESS_CONFIG_H
#define ACCESS_CONFIG_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "access_control.h"
#include "crypto_hash.h"

#define ACCESS_CONFIG_MAX_PIN_LENGTH 8U

void AccessConfig_Init(void);
size_t AccessConfig_LoadAuthorizedCards(AccessControl *control);
bool AccessConfig_IsPinAuthorized(const char *pin);
bool AccessConfig_SetPin(const char *pin);

#ifdef __cplusplus
}
#endif

#endif