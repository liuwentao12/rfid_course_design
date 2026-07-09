#include "access_config.h"

#include <string.h>

#include "crypto_hash.h"

/* 默认 PIN "123456" 对应的盐和哈希。
 * 盐固定为零（编译时常量），哈希在 AccessConfig_Init() 中计算。 */
static uint8_t pin_salt[CRYPTO_SALT_LEN];
static uint8_t pin_hash[CRYPTO_HASH_LEN];
static bool pin_configured;

void AccessConfig_Init(void)
{
  /* 用零盐对默认 PIN 做哈希，使初始状态不存明文 */
  (void)memset(pin_salt, 0, sizeof(pin_salt));
  Crypto_HashPin(pin_salt, "123456", pin_hash);
  pin_configured = true;
}

size_t AccessConfig_LoadAuthorizedCards(AccessControl *control)
{
  (void)control;
  return 0U;
}

bool AccessConfig_IsPinAuthorized(const char *pin)
{
  if (!pin_configured || pin == NULL)
  {
    return false;
  }
  return Crypto_VerifyPin(pin_salt, pin_hash, pin);
}

bool AccessConfig_SetPin(const char *pin)
{
  size_t length = (pin == NULL) ? 0U : strlen(pin);

  if (length == 0U || length > ACCESS_CONFIG_MAX_PIN_LENGTH)
  {
    return false;
  }

  Crypto_GenerateSalt(pin_salt);
  Crypto_HashPin(pin_salt, pin, pin_hash);
  pin_configured = true;
  return true;
}
