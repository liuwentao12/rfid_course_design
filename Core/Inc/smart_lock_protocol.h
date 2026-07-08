#ifndef SMART_LOCK_PROTOCOL_H
#define SMART_LOCK_PROTOCOL_H

#ifdef __cplusplus
extern "C" {
#endif

#include <stddef.h>
#include <stdint.h>

#define SMART_LOCK_PROTOCOL_SOF0 0xA5U
#define SMART_LOCK_PROTOCOL_SOF1 0x5AU
#define SMART_LOCK_PROTOCOL_VERSION 0x01U
#define SMART_LOCK_PROTOCOL_MAX_PAYLOAD_LENGTH 48U
#define SMART_LOCK_PROTOCOL_HEADER_LENGTH 7U
#define SMART_LOCK_PROTOCOL_CRC_LENGTH 2U
#define SMART_LOCK_PROTOCOL_MAX_FRAME_LENGTH \
  (SMART_LOCK_PROTOCOL_HEADER_LENGTH + SMART_LOCK_PROTOCOL_MAX_PAYLOAD_LENGTH + SMART_LOCK_PROTOCOL_CRC_LENGTH)

typedef enum {
  SMART_LOCK_MSG_ACK = 0x01U,
  SMART_LOCK_MSG_NACK = 0x02U,
  SMART_LOCK_MSG_HELLO = 0x10U,
  SMART_LOCK_MSG_STATUS_QUERY = 0x11U,
  SMART_LOCK_MSG_STATUS_REPORT = 0x12U,
  SMART_LOCK_MSG_CARD_ADD = 0x20U,
  SMART_LOCK_MSG_CARD_REMOVE = 0x21U,
  SMART_LOCK_MSG_CARD_SYNC = 0x22U,
  SMART_LOCK_MSG_AUTH_EVENT = 0x30U,
  SMART_LOCK_MSG_CAPTURE_ALERT = 0x31U
} SmartLockProtocol_MessageType;

typedef enum {
  SMART_LOCK_PROTOCOL_OK = 0,
  SMART_LOCK_PROTOCOL_IN_PROGRESS,
  SMART_LOCK_PROTOCOL_FRAME_READY,
  SMART_LOCK_PROTOCOL_INVALID_ARGUMENT,
  SMART_LOCK_PROTOCOL_BUFFER_TOO_SMALL,
  SMART_LOCK_PROTOCOL_PAYLOAD_TOO_LONG,
  SMART_LOCK_PROTOCOL_BAD_VERSION,
  SMART_LOCK_PROTOCOL_BAD_CRC
} SmartLockProtocol_Status;

typedef struct {
  uint8_t type;
  uint8_t sequence;
  uint16_t payload_length;
  uint8_t payload[SMART_LOCK_PROTOCOL_MAX_PAYLOAD_LENGTH];
} SmartLockProtocol_Frame;

typedef enum {
  SMART_LOCK_RX_WAIT_SOF0 = 0,
  SMART_LOCK_RX_WAIT_SOF1,
  SMART_LOCK_RX_VERSION,
  SMART_LOCK_RX_TYPE,
  SMART_LOCK_RX_SEQUENCE,
  SMART_LOCK_RX_LENGTH_LO,
  SMART_LOCK_RX_LENGTH_HI,
  SMART_LOCK_RX_PAYLOAD,
  SMART_LOCK_RX_CRC_LO,
  SMART_LOCK_RX_CRC_HI
} SmartLockProtocol_RxState;

typedef struct {
  SmartLockProtocol_RxState state;
  SmartLockProtocol_Frame frame;
  uint16_t crc;
  uint16_t received_crc;
  uint16_t payload_index;
} SmartLockProtocol_Decoder;

uint16_t SmartLockProtocol_Crc16(const uint8_t *data, size_t length);
SmartLockProtocol_Status SmartLockProtocol_BuildFrame(uint8_t type, uint8_t sequence, const uint8_t *payload, uint16_t payload_length, uint8_t *output, size_t output_capacity, size_t *output_length);
void SmartLockProtocol_DecoderInit(SmartLockProtocol_Decoder *decoder);
SmartLockProtocol_Status SmartLockProtocol_DecodeByte(SmartLockProtocol_Decoder *decoder, uint8_t byte, SmartLockProtocol_Frame *output_frame);

#ifdef __cplusplus
}
#endif

#endif