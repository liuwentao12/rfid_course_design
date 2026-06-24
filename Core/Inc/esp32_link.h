#ifndef ESP32_LINK_H
#define ESP32_LINK_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "smart_lock_protocol.h"
#include "stm32f1xx_hal.h"

#define ESP32_LINK_TX_QUEUE_DEPTH 4U
#define ESP32_LINK_MAX_UID_LENGTH 10U

typedef enum
{
  ESP32_LINK_OK = 0,
  ESP32_LINK_INVALID_ARGUMENT,
  ESP32_LINK_QUEUE_FULL,
  ESP32_LINK_BUILD_FAILED
} ESP32Link_Status;

typedef enum
{
  ESP32_LINK_AUTH_METHOD_PIN = 1U,
  ESP32_LINK_AUTH_METHOD_NFC = 2U
} ESP32Link_AuthMethod;

typedef enum
{
  ESP32_LINK_AUTH_DENIED = 0U,
  ESP32_LINK_AUTH_GRANTED = 1U
} ESP32Link_AuthResult;

typedef enum
{
  ESP32_LINK_ALERT_FAILURE_THRESHOLD = 1U
} ESP32Link_AlertReason;

typedef struct
{
  uint8_t bytes[SMART_LOCK_PROTOCOL_MAX_FRAME_LENGTH];
  uint16_t length;
  uint8_t sequence;
  uint8_t type;
  uint8_t attempts;
} ESP32Link_TxFrame;

typedef struct
{
  UART_HandleTypeDef *uart;
  SmartLockProtocol_Decoder decoder;
  uint8_t next_sequence;

  ESP32Link_TxFrame queue[ESP32_LINK_TX_QUEUE_DEPTH];
  uint8_t queue_head;
  uint8_t queue_tail;
  uint8_t queue_count;

  ESP32Link_TxFrame active_frame;
  bool waiting_ack;
  uint32_t active_last_tx_ms;

  uint32_t tx_acked_count;
  uint32_t tx_dropped_count;
  uint32_t rx_frame_count;
  uint8_t remote_status;
  bool remote_online;
} ESP32Link_HandleTypeDef;

void ESP32Link_Init(ESP32Link_HandleTypeDef *link, UART_HandleTypeDef *uart);

void ESP32Link_Poll(ESP32Link_HandleTypeDef *link);

ESP32Link_Status ESP32Link_QueueHello(ESP32Link_HandleTypeDef *link);

ESP32Link_Status ESP32Link_QueueStatusQuery(ESP32Link_HandleTypeDef *link);

ESP32Link_Status ESP32Link_QueueAuthEvent(ESP32Link_HandleTypeDef *link,
                                          ESP32Link_AuthMethod method,
                                          ESP32Link_AuthResult result,
                                          uint8_t failure_count,
                                          const uint8_t *uid,
                                          uint8_t uid_length);

ESP32Link_Status ESP32Link_QueueCaptureAlert(ESP32Link_HandleTypeDef *link,
                                             ESP32Link_AlertReason reason,
                                             uint8_t failure_count,
                                             const uint8_t *uid,
                                             uint8_t uid_length);

bool ESP32Link_IsRemoteOnline(const ESP32Link_HandleTypeDef *link);

#ifdef __cplusplus
}
#endif

#endif
