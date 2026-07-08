#include "smart_lock_protocol.h"

#include <string.h>

#define SMART_LOCK_CRC_INITIAL 0xFFFFU
#define SMART_LOCK_CRC_POLY 0x1021U

static uint16_t SmartLockProtocol_Crc16Update(uint16_t crc, uint8_t byte)
{
  crc ^= (uint16_t)byte << 8U;

  for (uint8_t bit = 0U; bit < 8U; bit++)
  {
    crc = (uint16_t)((crc & 0x8000U) != 0U ? (crc << 1U) ^ SMART_LOCK_CRC_POLY : crc << 1U);
  }

  return crc;
}

uint16_t SmartLockProtocol_Crc16(const uint8_t *data, size_t length)
{
  uint16_t crc = SMART_LOCK_CRC_INITIAL;

  if (data == NULL && length > 0U)
  {
    return 0U;
  }

  for (size_t i = 0U; i < length; i++)
  {
    crc = SmartLockProtocol_Crc16Update(crc, data[i]);
  }

  return crc;
}

SmartLockProtocol_Status SmartLockProtocol_BuildFrame(uint8_t type, uint8_t sequence, const uint8_t *payload, uint16_t payload_length, uint8_t *output, size_t output_capacity, size_t *output_length)
{
  size_t frame_length;
  uint16_t crc;
  size_t offset = 0U;

  if (output == NULL || output_length == NULL || (payload == NULL && payload_length > 0U))
  {
    return SMART_LOCK_PROTOCOL_INVALID_ARGUMENT;
  }

  if (payload_length > SMART_LOCK_PROTOCOL_MAX_PAYLOAD_LENGTH)
  {
    return SMART_LOCK_PROTOCOL_PAYLOAD_TOO_LONG;
  }

  frame_length = SMART_LOCK_PROTOCOL_HEADER_LENGTH + (size_t)payload_length + SMART_LOCK_PROTOCOL_CRC_LENGTH;
  if (output_capacity < frame_length)
  {
    return SMART_LOCK_PROTOCOL_BUFFER_TOO_SMALL;
  }

  output[offset++] = SMART_LOCK_PROTOCOL_SOF0;
  output[offset++] = SMART_LOCK_PROTOCOL_SOF1;
  output[offset++] = SMART_LOCK_PROTOCOL_VERSION;
  output[offset++] = type;
  output[offset++] = sequence;
  output[offset++] = (uint8_t)(payload_length & 0xFFU);
  output[offset++] = (uint8_t)((payload_length >> 8U) & 0xFFU);

  if (payload_length > 0U)
  {
    memcpy(&output[offset], payload, payload_length);
    offset += payload_length;
  }

  crc = SmartLockProtocol_Crc16(&output[2], (size_t)(5U + payload_length));
  output[offset++] = (uint8_t)(crc & 0xFFU);
  output[offset++] = (uint8_t)((crc >> 8U) & 0xFFU);

  *output_length = offset;
  return SMART_LOCK_PROTOCOL_OK;
}

static void SmartLockProtocol_DecoderReset(SmartLockProtocol_Decoder *decoder)
{
  decoder->state = SMART_LOCK_RX_WAIT_SOF0;
  decoder->crc = SMART_LOCK_CRC_INITIAL;
  decoder->received_crc = 0U;
  decoder->payload_index = 0U;
  memset(&decoder->frame, 0, sizeof(decoder->frame));
}

void SmartLockProtocol_DecoderInit(SmartLockProtocol_Decoder *decoder)
{
  if (decoder == NULL)
  {
    return;
  }

  SmartLockProtocol_DecoderReset(decoder);
}

SmartLockProtocol_Status SmartLockProtocol_DecodeByte(SmartLockProtocol_Decoder *decoder, uint8_t byte, SmartLockProtocol_Frame *output_frame)
{
  if (decoder == NULL || output_frame == NULL)
  {
    return SMART_LOCK_PROTOCOL_INVALID_ARGUMENT;
  }

  switch (decoder->state)
  {
    case SMART_LOCK_RX_WAIT_SOF0:
      if (byte == SMART_LOCK_PROTOCOL_SOF0)
      {
        decoder->state = SMART_LOCK_RX_WAIT_SOF1;
      }
      break;

    case SMART_LOCK_RX_WAIT_SOF1:
      if (byte == SMART_LOCK_PROTOCOL_SOF1)
      {
        decoder->state = SMART_LOCK_RX_VERSION;
        decoder->crc = SMART_LOCK_CRC_INITIAL;
      }
      else if (byte != SMART_LOCK_PROTOCOL_SOF0)
      {
        SmartLockProtocol_DecoderReset(decoder);
      }
      break;

    case SMART_LOCK_RX_VERSION:
      if (byte != SMART_LOCK_PROTOCOL_VERSION)
      {
        SmartLockProtocol_DecoderReset(decoder);
        return SMART_LOCK_PROTOCOL_BAD_VERSION;
      }
      decoder->crc = SmartLockProtocol_Crc16Update(decoder->crc, byte);
      decoder->state = SMART_LOCK_RX_TYPE;
      break;

    case SMART_LOCK_RX_TYPE:
      decoder->frame.type = byte;
      decoder->crc = SmartLockProtocol_Crc16Update(decoder->crc, byte);
      decoder->state = SMART_LOCK_RX_SEQUENCE;
      break;

    case SMART_LOCK_RX_SEQUENCE:
      decoder->frame.sequence = byte;
      decoder->crc = SmartLockProtocol_Crc16Update(decoder->crc, byte);
      decoder->state = SMART_LOCK_RX_LENGTH_LO;
      break;

    case SMART_LOCK_RX_LENGTH_LO:
      decoder->frame.payload_length = byte;
      decoder->crc = SmartLockProtocol_Crc16Update(decoder->crc, byte);
      decoder->state = SMART_LOCK_RX_LENGTH_HI;
      break;

    case SMART_LOCK_RX_LENGTH_HI:
      decoder->frame.payload_length |= (uint16_t)byte << 8U;
      decoder->crc = SmartLockProtocol_Crc16Update(decoder->crc, byte);
      if (decoder->frame.payload_length > SMART_LOCK_PROTOCOL_MAX_PAYLOAD_LENGTH)
      {
        SmartLockProtocol_DecoderReset(decoder);
        return SMART_LOCK_PROTOCOL_PAYLOAD_TOO_LONG;
      }
      decoder->state = (decoder->frame.payload_length == 0U) ? SMART_LOCK_RX_CRC_LO : SMART_LOCK_RX_PAYLOAD;
      break;

    case SMART_LOCK_RX_PAYLOAD:
      decoder->frame.payload[decoder->payload_index++] = byte;
      decoder->crc = SmartLockProtocol_Crc16Update(decoder->crc, byte);
      if (decoder->payload_index >= decoder->frame.payload_length)
      {
        decoder->state = SMART_LOCK_RX_CRC_LO;
      }
      break;

    case SMART_LOCK_RX_CRC_LO:
      decoder->received_crc = byte;
      decoder->state = SMART_LOCK_RX_CRC_HI;
      break;

    case SMART_LOCK_RX_CRC_HI:
      decoder->received_crc |= (uint16_t)byte << 8U;
      if (decoder->received_crc == decoder->crc)
      {
        *output_frame = decoder->frame;
        SmartLockProtocol_DecoderReset(decoder);
        return SMART_LOCK_PROTOCOL_FRAME_READY;
      }

      SmartLockProtocol_DecoderReset(decoder);
      return SMART_LOCK_PROTOCOL_BAD_CRC;

    default:
      SmartLockProtocol_DecoderReset(decoder);
      break;
  }

  return SMART_LOCK_PROTOCOL_IN_PROGRESS;
}
