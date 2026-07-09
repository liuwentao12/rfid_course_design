#ifndef CRYPTO_HASH_H
#define CRYPTO_HASH_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stdint.h>

#define CRYPTO_SALT_LEN 8U
#define CRYPTO_HASH_LEN 32U /* SHA-256 输出长度 */

/* 用 STM32 唯一 ID + 系统时钟生成随机盐 */
void Crypto_GenerateSalt(uint8_t salt[CRYPTO_SALT_LEN]);

/* 计算 SHA-256(salt || pin) */
void Crypto_HashPin(const uint8_t salt[CRYPTO_SALT_LEN], const char *pin,
                    uint8_t hash[CRYPTO_HASH_LEN]);

/* 验证 PIN: 重新计算 hash 并与 stored_hash 比较 */
bool Crypto_VerifyPin(const uint8_t salt[CRYPTO_SALT_LEN],
                      const uint8_t stored_hash[CRYPTO_HASH_LEN],
                      const char *pin);

#ifdef __cplusplus
}
#endif

#endif
