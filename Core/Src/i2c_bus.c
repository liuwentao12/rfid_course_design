#include "i2c_bus.h"

#include <stdbool.h>

static osMutexId_t i2c_bus_mutex;

void I2CBus_Init(osMutexId_t mutex)
{
  i2c_bus_mutex = mutex;
}

static bool I2CBus_ShouldLock(void)
{
  return i2c_bus_mutex != NULL && osKernelGetState() == osKernelRunning;
}

static HAL_StatusTypeDef I2CBus_Lock(bool *locked)
{
  *locked = false;

  if (!I2CBus_ShouldLock())
  {
    return HAL_OK;
  }

  if (osMutexAcquire(i2c_bus_mutex, osWaitForever) != osOK)
  {
    return HAL_BUSY;
  }

  *locked = true;
  return HAL_OK;
}

static void I2CBus_Unlock(bool locked)
{
  if (locked)
  {
    (void)osMutexRelease(i2c_bus_mutex);
  }
}

HAL_StatusTypeDef I2CBus_IsDeviceReady(I2C_HandleTypeDef *hi2c,
                                       uint16_t address,
                                       uint32_t trials,
                                       uint32_t timeout)
{
  bool locked;
  HAL_StatusTypeDef status = I2CBus_Lock(&locked);

  if (status == HAL_OK)
  {
    status = HAL_I2C_IsDeviceReady(hi2c, address, trials, timeout);
    I2CBus_Unlock(locked);
  }

  return status;
}

HAL_StatusTypeDef I2CBus_MasterTransmit(I2C_HandleTypeDef *hi2c,
                                        uint16_t address,
                                        const uint8_t *data,
                                        uint16_t size,
                                        uint32_t timeout)
{
  bool locked;
  HAL_StatusTypeDef status = I2CBus_Lock(&locked);

  if (status == HAL_OK)
  {
    status = HAL_I2C_Master_Transmit(hi2c, address, (uint8_t *)data, size, timeout);
    I2CBus_Unlock(locked);
  }

  return status;
}

HAL_StatusTypeDef I2CBus_MasterReceive(I2C_HandleTypeDef *hi2c,
                                       uint16_t address,
                                       uint8_t *data,
                                       uint16_t size,
                                       uint32_t timeout)
{
  bool locked;
  HAL_StatusTypeDef status = I2CBus_Lock(&locked);

  if (status == HAL_OK)
  {
    status = HAL_I2C_Master_Receive(hi2c, address, data, size, timeout);
    I2CBus_Unlock(locked);
  }

  return status;
}