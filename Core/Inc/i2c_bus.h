#ifndef I2C_BUS_H
#define I2C_BUS_H

#ifdef __cplusplus
extern "C" {
#endif

#include "cmsis_os.h"
#include "stm32f1xx_hal.h"

void I2CBus_Init(osMutexId_t mutex);
HAL_StatusTypeDef I2CBus_IsDeviceReady(I2C_HandleTypeDef *hi2c, uint16_t address, uint32_t trials, uint32_t timeout);
HAL_StatusTypeDef I2CBus_MasterTransmit(I2C_HandleTypeDef *hi2c, uint16_t address, const uint8_t *data, uint16_t size, uint32_t timeout);
HAL_StatusTypeDef I2CBus_MasterReceive(I2C_HandleTypeDef *hi2c, uint16_t address, uint8_t *data, uint16_t size, uint32_t timeout);

#ifdef __cplusplus
}
#endif

#endif