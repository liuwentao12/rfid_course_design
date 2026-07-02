#ifndef PN532_H
#define PN532_H

#ifdef __cplusplus
extern "C" {
#endif

#include "stm32f1xx_hal.h"

#define PN532_DEFAULT_I2C_ADDRESS (0x24U << 1)
#define PN532_MAX_UID_LENGTH 10U

typedef struct {
  I2C_HandleTypeDef *hi2c;
  uint16_t address;
} PN532_HandleTypeDef;

HAL_StatusTypeDef PN532_Init(PN532_HandleTypeDef *device, I2C_HandleTypeDef *hi2c, uint16_t address);
HAL_StatusTypeDef PN532_GetFirmwareVersion(PN532_HandleTypeDef *device, uint32_t *version);
HAL_StatusTypeDef PN532_SAMConfig(PN532_HandleTypeDef *device);
HAL_StatusTypeDef PN532_ReadCardUID(PN532_HandleTypeDef *device, uint8_t *uid, uint8_t *uid_len);

#ifdef __cplusplus
}
#endif

#endif