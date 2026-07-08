#include "esp32_link.h"

#include <string.h>

#define ESP32_LINK_ACK_TIMEOUT_MS 350U
#define ESP32_LINK_MAX_ATTEMPTS 3U
#define ESP32_LINK_UART_TX_TIMEOUT_MS 30U
#define ESP32_LINK_RX_BUDGET_BYTES 32U

#define ESP32_LINK_ACK_OK 0x00U
#define ESP32_LINK_NACK_UNSUPPORTED 0x02U

static void ESP32Link_PutU32Le(uint8_t *output, uint32_t value)
{
  output[0] = (uint8_t)(value & 0xFFU);
  output[1] = (uint8_t)((value >> 8U) & 0xFFU);
  output[2] = (uint8_t)((value >> 16U) & 0xFFU);
  output[3] = (uint8_t)((value >> 24U) & 0xFFU);
}

static ESP32Link_Status ESP32Link_QueueFrame(ESP32Link_HandleTypeDef *link, uint8_t type, const uint8_t *payload, uint16_t payload_length)
{
  ESP32Link_TxFrame *frame;
  size_t encoded_length = 0U;
  SmartLockProtocol_Status status;

  if (link == NULL || (payload == NULL && payload_length > 0U))
  {
    return ESP32_LINK_INVALID_ARGUMENT;
  }

  if (link->queue_count >= ESP32_LINK_TX_QUEUE_DEPTH)
  {
    return ESP32_LINK_QUEUE_FULL;
  }

  frame = &link->queue[link->queue_tail];
  memset(frame, 0, sizeof(*frame));
  frame->sequence = link->next_sequence++;
  frame->type = type;

  status = SmartLockProtocol_BuildFrame(type,
                                        frame->sequence,
                                        payload,
                                        payload_length,
                                        frame->bytes,
                                        sizeof(frame->bytes),
                                        &encoded_length);
  if (status != SMART_LOCK_PROTOCOL_OK || encoded_length > UINT16_MAX)
  {
    return ESP32_LINK_BUILD_FAILED;
  }

  frame->length = (uint16_t)encoded_length;
  link->queue_tail = (uint8_t)((link->queue_tail + 1U) % ESP32_LINK_TX_QUEUE_DEPTH);
  link->queue_count++;
  return ESP32_LINK_OK;
}

static void ESP32Link_SendImmediate(ESP32Link_HandleTypeDef *link, uint8_t type, uint8_t sequence, const uint8_t *payload, uint16_t payload_length)
{
  uint8_t bytes[SMART_LOCK_PROTOCOL_MAX_FRAME_LENGTH];
  size_t length = 0U;

  if (link == NULL || link->uart == NULL)
  {
    return;
  }

  if (SmartLockProtocol_BuildFrame(type,
                                   sequence,
                                   payload,
                                   payload_length,
                                   bytes,
                                   sizeof(bytes),
                                   &length) == SMART_LOCK_PROTOCOL_OK)
  {
    (void)HAL_UART_Transmit(link->uart, bytes, (uint16_t)length, ESP32_LINK_UART_TX_TIMEOUT_MS);
  }
}

static void ESP32Link_SendAck(ESP32Link_HandleTypeDef *link, uint8_t sequence)
{
  const uint8_t payload[] = {ESP32_LINK_ACK_OK};
  ESP32Link_SendImmediate(link, SMART_LOCK_MSG_ACK, sequence, payload, sizeof(payload));
}

static void ESP32Link_SendNack(ESP32Link_HandleTypeDef *link, uint8_t sequence, uint8_t code)
{
  const uint8_t payload[] = {code};
  ESP32Link_SendImmediate(link, SMART_LOCK_MSG_NACK, sequence, payload, sizeof(payload));
}

static void ESP32Link_StartNextFrame(ESP32Link_HandleTypeDef *link)
{
  if (link->waiting_ack || link->queue_count == 0U)
  {
    return;
  }

  link->active_frame = link->queue[link->queue_head];
  link->queue_head = (uint8_t)((link->queue_head + 1U) % ESP32_LINK_TX_QUEUE_DEPTH);
  link->queue_count--;
  link->active_frame.attempts = 0U;
  link->waiting_ack = true;
}

static void ESP32Link_TransmitActiveFrame(ESP32Link_HandleTypeDef *link)
{
  if (!link->waiting_ack || link->uart == NULL)
  {
    return;
  }

  if (HAL_UART_Transmit(link->uart,
                        link->active_frame.bytes,
                        link->active_frame.length,
                        ESP32_LINK_UART_TX_TIMEOUT_MS) == HAL_OK)
  {
    link->active_frame.attempts++;
    link->active_last_tx_ms = HAL_GetTick();
  }
  else
  {
    link->active_frame.attempts = ESP32_LINK_MAX_ATTEMPTS;
    link->active_last_tx_ms = HAL_GetTick();
  }
}

static void ESP32Link_ServiceTx(ESP32Link_HandleTypeDef *link)
{
  uint32_t now;

  ESP32Link_StartNextFrame(link);

  if (!link->waiting_ack)
  {
    return;
  }

  now = HAL_GetTick();

  if (link->active_frame.attempts == 0U)
  {
    ESP32Link_TransmitActiveFrame(link);
    return;
  }

  if ((uint32_t)(now - link->active_last_tx_ms) < ESP32_LINK_ACK_TIMEOUT_MS)
  {
    return;
  }

  if (link->active_frame.attempts >= ESP32_LINK_MAX_ATTEMPTS)
  {
    link->waiting_ack = false;
    link->tx_dropped_count++;
    return;
  }

  ESP32Link_TransmitActiveFrame(link);
}

static void ESP32Link_HandleAck(ESP32Link_HandleTypeDef *link, const SmartLockProtocol_Frame *frame)
{
  if (link->waiting_ack && frame->sequence == link->active_frame.sequence)
  {
    link->waiting_ack = false;
    link->tx_acked_count++;
    link->remote_online = true;
  }
}

static void ESP32Link_HandleNack(ESP32Link_HandleTypeDef *link, const SmartLockProtocol_Frame *frame)
{
  if (link->waiting_ack && frame->sequence == link->active_frame.sequence)
  {
    if (link->active_frame.attempts < ESP32_LINK_MAX_ATTEMPTS)
    {
      link->active_last_tx_ms = 0U;
    }
    else
    {
      link->waiting_ack = false;
      link->tx_dropped_count++;
    }
  }
}

static void ESP32Link_HandleFrame(ESP32Link_HandleTypeDef *link, const SmartLockProtocol_Frame *frame)
{
  link->rx_frame_count++;

  switch (frame->type)
  {
    case SMART_LOCK_MSG_ACK:
      ESP32Link_HandleAck(link, frame);
      break;

    case SMART_LOCK_MSG_NACK:
      ESP32Link_HandleNack(link, frame);
      break;

    case SMART_LOCK_MSG_HELLO:
      link->remote_online = true;
      ESP32Link_SendAck(link, frame->sequence);
      break;

    case SMART_LOCK_MSG_STATUS_REPORT:
      if (frame->payload_length >= 1U)
      {
        link->remote_status = frame->payload[0];
      }
      link->remote_online = true;
      ESP32Link_SendAck(link, frame->sequence);
      break;

    default:
      ESP32Link_SendNack(link, frame->sequence, ESP32_LINK_NACK_UNSUPPORTED);
      break;
  }
}

void ESP32Link_Init(ESP32Link_HandleTypeDef *link, UART_HandleTypeDef *uart)
{
  if (link == NULL)
  {
    return;
  }

  memset(link, 0, sizeof(*link));
  link->uart = uart;
  link->next_sequence = 1U;
  SmartLockProtocol_DecoderInit(&link->decoder);
}

void ESP32Link_Poll(ESP32Link_HandleTypeDef *link)
{
  uint8_t byte;
  SmartLockProtocol_Frame frame;

  if (link == NULL || link->uart == NULL)
  {
    return;
  }

  for (uint8_t i = 0U; i < ESP32_LINK_RX_BUDGET_BYTES; i++)
  {
    if (HAL_UART_Receive(link->uart, &byte, 1U, 0U) != HAL_OK)
    {
      break;
    }

    if (SmartLockProtocol_DecodeByte(&link->decoder, byte, &frame) == SMART_LOCK_PROTOCOL_FRAME_READY)
    {
      ESP32Link_HandleFrame(link, &frame);
    }
  }

  ESP32Link_ServiceTx(link);
}

ESP32Link_Status ESP32Link_QueueHello(ESP32Link_HandleTypeDef *link)
{
  const uint8_t payload[] = {'S', 'T', 'M', '3', '2'};
  return ESP32Link_QueueFrame(link, SMART_LOCK_MSG_HELLO, payload, sizeof(payload));
}

ESP32Link_Status ESP32Link_QueueStatusQuery(ESP32Link_HandleTypeDef *link)
{
  return ESP32Link_QueueFrame(link, SMART_LOCK_MSG_STATUS_QUERY, NULL, 0U);
}

ESP32Link_Status ESP32Link_QueueAuthEvent(ESP32Link_HandleTypeDef *link, ESP32Link_AuthMethod method, ESP32Link_AuthResult result, uint8_t failure_count, const uint8_t *uid, uint8_t uid_length)
{
  uint8_t payload[8U + ESP32_LINK_MAX_UID_LENGTH];

  if (uid_length > ESP32_LINK_MAX_UID_LENGTH || (uid == NULL && uid_length > 0U))
  {
    return ESP32_LINK_INVALID_ARGUMENT;
  }

  payload[0] = (uint8_t)method;
  payload[1] = (uint8_t)result;
  payload[2] = failure_count;
  payload[3] = uid_length;
  ESP32Link_PutU32Le(&payload[4], HAL_GetTick());

  if (uid_length > 0U)
  {
    memcpy(&payload[8], uid, uid_length);
  }

  return ESP32Link_QueueFrame(link,
                              SMART_LOCK_MSG_AUTH_EVENT,
                              payload,
                              (uint16_t)(8U + uid_length));
}

ESP32Link_Status ESP32Link_QueueCaptureAlert(ESP32Link_HandleTypeDef *link, ESP32Link_AlertReason reason, uint8_t failure_count, const uint8_t *uid, uint8_t uid_length)
{
  uint8_t payload[8U + ESP32_LINK_MAX_UID_LENGTH];

  if (uid_length > ESP32_LINK_MAX_UID_LENGTH || (uid == NULL && uid_length > 0U))
  {
    return ESP32_LINK_INVALID_ARGUMENT;
  }

  payload[0] = (uint8_t)reason;
  payload[1] = failure_count;
  payload[2] = uid_length;
  payload[3] = 0U;
  ESP32Link_PutU32Le(&payload[4], HAL_GetTick());

  if (uid_length > 0U)
  {
    memcpy(&payload[8], uid, uid_length);
  }

  return ESP32Link_QueueFrame(link,
                              SMART_LOCK_MSG_CAPTURE_ALERT,
                              payload,
                              (uint16_t)(8U + uid_length));
}

bool ESP32Link_IsRemoteOnline(const ESP32Link_HandleTypeDef *link)
{
  return link != NULL && link->remote_online;
}
