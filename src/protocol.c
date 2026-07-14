#include "syneeg/protocol.h"

#include "syneeg/ads1299.h"

#include <string.h>

static void put_u16(uint8_t *out, uint16_t value) {
    out[0] = (uint8_t)(value >> 8u);
    out[1] = (uint8_t)value;
}

static void put_u32(uint8_t *out, uint32_t value) {
    out[0] = (uint8_t)(value >> 24u);
    out[1] = (uint8_t)(value >> 16u);
    out[2] = (uint8_t)(value >> 8u);
    out[3] = (uint8_t)value;
}

static void put_u64(uint8_t *out, uint64_t value) {
    for (size_t i = 0u; i < 8u; ++i) {
        out[i] = (uint8_t)(value >> (56u - (uint32_t)(i * 8u)));
    }
}

static uint16_t get_u16(const uint8_t *in) {
    return (uint16_t)(((uint16_t)in[0] << 8u) | (uint16_t)in[1]);
}

static uint32_t get_u32(const uint8_t *in) {
    return ((uint32_t)in[0] << 24u) | ((uint32_t)in[1] << 16u) |
           ((uint32_t)in[2] << 8u) | (uint32_t)in[3];
}

static uint64_t get_u64(const uint8_t *in) {
    uint64_t value = 0u;
    for (size_t i = 0u; i < 8u; ++i) {
        value = (value << 8u) | (uint64_t)in[i];
    }
    return value;
}

static bool channels_fit_signed24(const syneeg_sample_t *sample) {
    for (size_t i = 0u; i < SYNEEG_CHANNEL_COUNT; ++i) {
        if (sample->channel[i] < -8388608 || sample->channel[i] > 8388607) {
            return false;
        }
    }
    return true;
}

uint32_t syneeg_crc32c(const uint8_t *data, size_t length) {
    uint32_t crc = UINT32_C(0xFFFFFFFF);
    if (data == NULL) {
        return crc;
    }
    for (size_t i = 0u; i < length; ++i) {
        crc ^= (uint32_t)data[i];
        for (uint8_t bit = 0u; bit < 8u; ++bit) {
            const uint32_t mask = (uint32_t)(-(int32_t)(crc & 1u));
            crc = (crc >> 1u) ^ (UINT32_C(0x82F63B78) & mask);
        }
    }
    return ~crc;
}

syneeg_status_t syneeg_protocol_encode_sample(const syneeg_sample_t *sample,
                                              uint8_t *output,
                                              size_t capacity,
                                              size_t *written) {
    if (sample == NULL || output == NULL || written == NULL) {
        return SYNEEG_ERROR_ARGUMENT;
    }
    if (!channels_fit_signed24(sample)) {
        return SYNEEG_ERROR_FORMAT;
    }
    if (capacity < SYNEEG_NATIVE_SAMPLE_BYTES) {
        return SYNEEG_ERROR_CAPACITY;
    }

    memset(output, 0, SYNEEG_NATIVE_SAMPLE_BYTES);
    output[0] = SYNEEG_NATIVE_VERSION;
    output[1] = (uint8_t)SYNEEG_FRAME_SAMPLE;
    output[2] = sample->flags;
    output[3] = 24u;
    put_u32(&output[4], sample->node_id);
    put_u32(&output[8], sample->segment_id);
    put_u32(&output[12], sample->sequence);
    put_u64(&output[16], sample->epoch_tick);
    memcpy(&output[24], sample->ads_status, 3u);
    for (size_t i = 0u; i < SYNEEG_CHANNEL_COUNT; ++i) {
        const uint32_t raw = (uint32_t)sample->channel[i] & 0x00FFFFFFu;
        output[27u + (i * 3u)] = (uint8_t)(raw >> 16u);
        output[28u + (i * 3u)] = (uint8_t)(raw >> 8u);
        output[29u + (i * 3u)] = (uint8_t)raw;
    }
    output[51] = sample->clock_quality;
    put_u16(&output[52], sample->configuration_generation);
    put_u32(&output[54], sample->topology_generation);
    put_u16(&output[58], sample->gaps_before);
    put_u32(&output[60], syneeg_crc32c(output, 60u));
    *written = SYNEEG_NATIVE_SAMPLE_BYTES;
    return SYNEEG_OK;
}

syneeg_status_t syneeg_protocol_decode_sample(const uint8_t *input,
                                              size_t length,
                                              syneeg_sample_t *sample) {
    if (input == NULL || sample == NULL) {
        return SYNEEG_ERROR_ARGUMENT;
    }
    if (length != SYNEEG_NATIVE_SAMPLE_BYTES || input[0] != SYNEEG_NATIVE_VERSION ||
        input[1] != (uint8_t)SYNEEG_FRAME_SAMPLE || input[3] != 24u) {
        return SYNEEG_ERROR_FORMAT;
    }
    if (get_u32(&input[60]) != syneeg_crc32c(input, 60u)) {
        return SYNEEG_ERROR_VERIFY;
    }

    memset(sample, 0, sizeof(*sample));
    sample->flags = input[2];
    sample->node_id = get_u32(&input[4]);
    sample->segment_id = get_u32(&input[8]);
    sample->sequence = get_u32(&input[12]);
    sample->epoch_tick = get_u64(&input[16]);
    memcpy(sample->ads_status, &input[24], 3u);
    for (size_t i = 0u; i < SYNEEG_CHANNEL_COUNT; ++i) {
        sample->channel[i] = syneeg_ads1299_sign_extend24(&input[27u + (i * 3u)]);
    }
    sample->clock_quality = input[51];
    sample->configuration_generation = get_u16(&input[52]);
    sample->topology_generation = get_u32(&input[54]);
    sample->gaps_before = get_u16(&input[58]);
    return SYNEEG_OK;
}

syneeg_status_t syneeg_protocol_encode_optical(const syneeg_optical_result_t *result,
                                               uint8_t *output,
                                               size_t capacity,
                                               size_t *written) {
    if (result == NULL || output == NULL || written == NULL) {
        return SYNEEG_ERROR_ARGUMENT;
    }
    if (capacity < SYNEEG_NATIVE_OPTICAL_BYTES) {
        return SYNEEG_ERROR_CAPACITY;
    }
    if ((result->wavelength_nm != 735u && result->wavelength_nm != 850u) ||
        result->source_state > (uint8_t)SYNEEG_OPTICAL_SOURCE_FAULT) {
        return SYNEEG_ERROR_FORMAT;
    }

    memset(output, 0, SYNEEG_NATIVE_OPTICAL_BYTES);
    output[0] = SYNEEG_NATIVE_OPTICAL_VERSION;
    output[1] = (uint8_t)SYNEEG_FRAME_OPTICAL_RESULT;
    output[3] = SYNEEG_OPTICAL_DETECTOR_COUNT;
    put_u32(&output[4], result->node_id);
    put_u32(&output[8], result->segment_id);
    put_u32(&output[12], result->sequence);
    put_u64(&output[16], result->carrier_tick);
    output[24] = result->source_id;
    output[25] = result->source_state;
    put_u16(&output[26], result->wavelength_nm);
    put_u32(&output[28], result->accumulation_count);
    memcpy(&output[32], result->detector_id, SYNEEG_OPTICAL_DETECTOR_COUNT);
    for (size_t i = 0u; i < SYNEEG_OPTICAL_DETECTOR_COUNT; ++i) {
        put_u32(&output[36u + (i * 4u)], (uint32_t)result->accumulated_raw[i]);
    }
    output[52] = result->saturation_mask;
    output[53] = result->coupling_mask;
    output[54] = result->fault_evidence;
    output[55] = result->clock_quality;
    put_u16(&output[56], result->configuration_generation);
    put_u16(&output[58], result->gaps_before);
    put_u32(&output[60], syneeg_crc32c(output, 60u));
    *written = SYNEEG_NATIVE_OPTICAL_BYTES;
    return SYNEEG_OK;
}

syneeg_status_t syneeg_protocol_decode_optical(const uint8_t *input,
                                               size_t length,
                                               syneeg_optical_result_t *result) {
    if (input == NULL || result == NULL) {
        return SYNEEG_ERROR_ARGUMENT;
    }
    if (length != SYNEEG_NATIVE_OPTICAL_BYTES ||
        input[0] != SYNEEG_NATIVE_OPTICAL_VERSION ||
        input[1] != (uint8_t)SYNEEG_FRAME_OPTICAL_RESULT ||
        input[3] != SYNEEG_OPTICAL_DETECTOR_COUNT) {
        return SYNEEG_ERROR_FORMAT;
    }
    if (get_u32(&input[60]) != syneeg_crc32c(input, 60u)) {
        return SYNEEG_ERROR_VERIFY;
    }

    memset(result, 0, sizeof(*result));
    result->node_id = get_u32(&input[4]);
    result->segment_id = get_u32(&input[8]);
    result->sequence = get_u32(&input[12]);
    result->carrier_tick = get_u64(&input[16]);
    result->source_id = input[24];
    result->source_state = input[25];
    result->wavelength_nm = get_u16(&input[26]);
    result->accumulation_count = get_u32(&input[28]);
    memcpy(result->detector_id, &input[32], SYNEEG_OPTICAL_DETECTOR_COUNT);
    for (size_t i = 0u; i < SYNEEG_OPTICAL_DETECTOR_COUNT; ++i) {
        result->accumulated_raw[i] = (int32_t)get_u32(&input[36u + (i * 4u)]);
    }
    result->saturation_mask = input[52];
    result->coupling_mask = input[53];
    result->fault_evidence = input[54];
    result->clock_quality = input[55];
    result->configuration_generation = get_u16(&input[56]);
    result->gaps_before = get_u16(&input[58]);
    if ((result->wavelength_nm != 735u && result->wavelength_nm != 850u) ||
        result->source_state > (uint8_t)SYNEEG_OPTICAL_SOURCE_FAULT) {
        return SYNEEG_ERROR_FORMAT;
    }
    return SYNEEG_OK;
}

syneeg_status_t syneeg_protocol_encode_openbci(const syneeg_sample_t *sample,
                                               uint8_t packet_type,
                                               uint8_t output[SYNEEG_OPENBCI_PACKET_BYTES]) {
    if (sample == NULL || output == NULL || packet_type > 0x0Fu) {
        return SYNEEG_ERROR_ARGUMENT;
    }
    if (!channels_fit_signed24(sample)) {
        return SYNEEG_ERROR_FORMAT;
    }
    output[0] = 0xA0u;
    output[1] = (uint8_t)sample->sequence;
    for (size_t i = 0u; i < SYNEEG_CHANNEL_COUNT; ++i) {
        const uint32_t raw = (uint32_t)sample->channel[i] & 0x00FFFFFFu;
        output[2u + (i * 3u)] = (uint8_t)(raw >> 16u);
        output[3u + (i * 3u)] = (uint8_t)(raw >> 8u);
        output[4u + (i * 3u)] = (uint8_t)raw;
    }
    memset(&output[26], 0, 6u);
    output[32] = (uint8_t)(0xC0u | packet_type);
    return SYNEEG_OK;
}
