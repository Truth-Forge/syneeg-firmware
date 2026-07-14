#ifndef SYNEEG_PROTOCOL_H
#define SYNEEG_PROTOCOL_H

#include <stddef.h>
#include <stdint.h>

#include "syneeg/sample.h"
#include "syneeg/status.h"

#define SYNEEG_NATIVE_VERSION 1u
#define SYNEEG_NATIVE_SAMPLE_BYTES 64u
#define SYNEEG_NATIVE_OPTICAL_VERSION 1u
#define SYNEEG_NATIVE_OPTICAL_BYTES 64u
#define SYNEEG_OPENBCI_PACKET_BYTES 33u

typedef enum {
    SYNEEG_FRAME_EEG_SAMPLE = 0x01,
    SYNEEG_FRAME_SAMPLE = SYNEEG_FRAME_EEG_SAMPLE,
    SYNEEG_FRAME_HEARTBEAT = 0x02,
    SYNEEG_FRAME_OPTICAL_RESULT = 0x03,
    SYNEEG_FRAME_STATUS = 0x04,
    SYNEEG_FRAME_DESCRIPTOR = 0x05,
    SYNEEG_FRAME_SEGMENT_START = 0x10,
    SYNEEG_FRAME_SEGMENT_STOP = 0x11,
    SYNEEG_FRAME_FAULT = 0x20
} syneeg_frame_type_t;

syneeg_status_t syneeg_protocol_encode_sample(const syneeg_sample_t *sample,
                                              uint8_t *output,
                                              size_t capacity,
                                              size_t *written);
syneeg_status_t syneeg_protocol_decode_sample(const uint8_t *input,
                                              size_t length,
                                              syneeg_sample_t *sample);
syneeg_status_t syneeg_protocol_encode_optical(const syneeg_optical_result_t *result,
                                               uint8_t *output,
                                               size_t capacity,
                                               size_t *written);
syneeg_status_t syneeg_protocol_decode_optical(const uint8_t *input,
                                               size_t length,
                                               syneeg_optical_result_t *result);
syneeg_status_t syneeg_protocol_encode_openbci(const syneeg_sample_t *sample,
                                               uint8_t packet_type,
                                               uint8_t output[SYNEEG_OPENBCI_PACKET_BYTES]);
uint32_t syneeg_crc32c(const uint8_t *data, size_t length);

#endif
