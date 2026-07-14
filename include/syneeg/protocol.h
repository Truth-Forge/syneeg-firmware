#ifndef SYNEEG_PROTOCOL_H
#define SYNEEG_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#include "syneeg/sample.h"
#include "syneeg/status.h"

#define SYNEEG_NATIVE_VERSION 1u
#define SYNEEG_NATIVE_SAMPLE_BYTES 64u
#define SYNEEG_OPENBCI_PACKET_BYTES 33u

typedef enum {
    SYNEEG_FRAME_SAMPLE = 0x01,
    SYNEEG_FRAME_HEARTBEAT = 0x02,
    SYNEEG_FRAME_EPOCH_PROPOSE = 0x10,
    SYNEEG_FRAME_EPOCH_COMMIT = 0x11,
    SYNEEG_FRAME_EPOCH_STOP = 0x12,
    SYNEEG_FRAME_FAULT = 0x20
} syneeg_frame_type_t;

syneeg_status_t syneeg_protocol_encode_sample(const syneeg_sample_t *sample,
                                              uint8_t *output,
                                              size_t capacity,
                                              size_t *written);
syneeg_status_t syneeg_protocol_decode_sample(const uint8_t *input,
                                              size_t length,
                                              syneeg_sample_t *sample);
syneeg_status_t syneeg_protocol_encode_openbci(const syneeg_sample_t *sample,
                                               uint8_t packet_type,
                                               uint8_t output[SYNEEG_OPENBCI_PACKET_BYTES]);
uint32_t syneeg_crc32c(const uint8_t *data, size_t length);

#endif
