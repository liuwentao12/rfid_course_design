#include "pn532.h"

#include <string.h>

#define PN532_HOST_TO_PN532 0xD4U
#define PN532_PN532_TO_HOST 0xD5U
#define PN532_STATUS_READY 0x01U
#define PN532_COMMAND_GET_FIRMWARE_VERSION 0x02U
#define PN532_COMMAND_SAM_CONFIGURATION 0x14U
#define PN532_COMMAND_IN_LIST_PASSIVE_TARGET 0x4AU
#define PN532_FRAME_MAX_LENGTH 32U

static HAL_StatusTypeDef PN532_WaitReady(PN532_HandleTypeDef *device, uint32_t timeout)
{
  uint8_t status;
  uint32_t start = HAL_GetTick();

  while ((HAL_GetTick() - start) < timeout)
  {
    if (HAL_I2C_Master_Receive(device->hi2c, device->address, &status, 1U, 20U) == HAL_OK &&
        status == PN532_STATUS_READY)
    {
      return HAL_OK;
    }

    HAL_Delay(10U);
  }

  return HAL_TIMEOUT;
}

static HAL_StatusTypeDef PN532_WriteCommand(PN532_HandleTypeDef *device,
                                            uint8_t command,
                                            const uint8_t *params,
                                            uint8_t params_len)
{
  uint8_t frame[PN532_FRAME_MAX_LENGTH];
  uint8_t data_len = params_len + 2U;
  uint8_t checksum = PN532_HOST_TO_PN532 + command;
  uint8_t frame_len = 0U;

  if (params_len > (PN532_FRAME_MAX_LENGTH - 9U) ||
      (params_len > 0U && params == NULL))
  {
    return HAL_ERROR;
  }

  frame[frame_len++] = 0x00U;
  frame[frame_len++] = 0x00U;
  frame[frame_len++] = 0xFFU;
  frame[frame_len++] = data_len;
  frame[frame_len++] = (uint8_t)(0x100U - data_len);
  frame[frame_len++] = PN532_HOST_TO_PN532;
  frame[frame_len++] = command;

  for (uint8_t i = 0U; i < params_len; i++)
  {
    frame[frame_len++] = params[i];
    checksum = (uint8_t)(checksum + params[i]);
  }

  frame[frame_len++] = (uint8_t)(0x100U - checksum);
  frame[frame_len++] = 0x00U;

  return HAL_I2C_Master_Transmit(device->hi2c, device->address, frame, frame_len, 100U);
}

static HAL_StatusTypeDef PN532_ReadAck(PN532_HandleTypeDef *device)
{
  static const uint8_t expected_ack[] = {0x00U, 0x00U, 0xFFU, 0x00U, 0xFFU, 0x00U};
  uint8_t ack[sizeof(expected_ack) + 1U];

  if (PN532_WaitReady(device, 1000U) != HAL_OK)
  {
    return HAL_TIMEOUT;
  }

  if (HAL_I2C_Master_Receive(device->hi2c, device->address, ack, sizeof(ack), 100U) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (ack[0] != PN532_STATUS_READY || memcmp(&ack[1], expected_ack, sizeof(expected_ack)) != 0)
  {
    return HAL_ERROR;
  }

  return HAL_OK;
}

static HAL_StatusTypeDef PN532_ReadResponse(PN532_HandleTypeDef *device,
                                            uint8_t command,
                                            uint8_t *data,
                                            uint8_t *data_len,
                                            uint32_t timeout)
{
  uint8_t response[PN532_FRAME_MAX_LENGTH];
  uint8_t payload_len;

  if (data == NULL || data_len == NULL)
  {
    return HAL_ERROR;
  }

  if (PN532_WaitReady(device, timeout) != HAL_OK)
  {
    return HAL_TIMEOUT;
  }

  if (HAL_I2C_Master_Receive(device->hi2c, device->address, response, sizeof(response), 200U) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (response[0] != PN532_STATUS_READY ||
      response[1] != 0x00U || response[2] != 0x00U || response[3] != 0xFFU ||
      (uint8_t)(response[4] + response[5]) != 0x00U ||
      response[6] != PN532_PN532_TO_HOST || response[7] != (uint8_t)(command + 1U) ||
      response[4] < 2U)
  {
    return HAL_ERROR;
  }

  payload_len = response[4] - 2U;
  if (payload_len > *data_len)
  {
    payload_len = *data_len;
  }

  memcpy(data, &response[8], payload_len);
  *data_len = payload_len;

  return HAL_OK;
}

static HAL_StatusTypeDef PN532_SendCommandAndRead(PN532_HandleTypeDef *device,
                                                  uint8_t command,
                                                  const uint8_t *params,
                                                  uint8_t params_len,
                                                  uint8_t *data,
                                                  uint8_t *data_len,
                                                  uint32_t timeout)
{
  if (PN532_WriteCommand(device, command, params, params_len) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (PN532_ReadAck(device) != HAL_OK)
  {
    return HAL_ERROR;
  }

  return PN532_ReadResponse(device, command, data, data_len, timeout);
}

HAL_StatusTypeDef PN532_Init(PN532_HandleTypeDef *device, I2C_HandleTypeDef *hi2c, uint16_t address)
{
  if (device == NULL || hi2c == NULL)
  {
    return HAL_ERROR;
  }

  device->hi2c = hi2c;
  device->address = address;

  return HAL_I2C_IsDeviceReady(device->hi2c, device->address, 2U, 20U);
}

HAL_StatusTypeDef PN532_GetFirmwareVersion(PN532_HandleTypeDef *device, uint32_t *version)
{
  uint8_t data[4];
  uint8_t data_len = sizeof(data);

  if (device == NULL || version == NULL)
  {
    return HAL_ERROR;
  }

  if (PN532_SendCommandAndRead(device,
                               PN532_COMMAND_GET_FIRMWARE_VERSION,
                               NULL,
                               0U,
                               data,
                               &data_len,
                               1000U) != HAL_OK ||
      data_len < sizeof(data))
  {
    return HAL_ERROR;
  }

  *version = ((uint32_t)data[0] << 24) |
             ((uint32_t)data[1] << 16) |
             ((uint32_t)data[2] << 8) |
             data[3];

  return HAL_OK;
}

HAL_StatusTypeDef PN532_SAMConfig(PN532_HandleTypeDef *device)
{
  const uint8_t params[] = {0x01U, 0x14U, 0x01U};
  uint8_t data[4];
  uint8_t data_len = sizeof(data);

  if (device == NULL)
  {
    return HAL_ERROR;
  }

  return PN532_SendCommandAndRead(device,
                                  PN532_COMMAND_SAM_CONFIGURATION,
                                  params,
                                  sizeof(params),
                                  data,
                                  &data_len,
                                  1000U);
}

HAL_StatusTypeDef PN532_ReadCardUID(PN532_HandleTypeDef *device, uint8_t *uid, uint8_t *uid_len)
{
  const uint8_t params[] = {0x01U, 0x00U};
  uint8_t data[24];
  uint8_t data_len = sizeof(data);
  uint8_t detected_uid_len;

  if (device == NULL || uid == NULL || uid_len == NULL || *uid_len == 0U)
  {
    return HAL_ERROR;
  }

  if (PN532_SendCommandAndRead(device,
                               PN532_COMMAND_IN_LIST_PASSIVE_TARGET,
                               params,
                               sizeof(params),
                               data,
                               &data_len,
                               1000U) != HAL_OK)
  {
    return HAL_ERROR;
  }

  if (data_len < 6U || data[0] == 0U)
  {
    return HAL_TIMEOUT;
  }

  detected_uid_len = data[5];
  if ((uint8_t)(6U + detected_uid_len) > data_len)
  {
    return HAL_ERROR;
  }

  if (detected_uid_len > *uid_len)
  {
    detected_uid_len = *uid_len;
  }

  memcpy(uid, &data[6], detected_uid_len);
  *uid_len = detected_uid_len;

  return HAL_OK;
}