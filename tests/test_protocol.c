#include "test.h"

#include <stdint.h>
#include <string.h>

#include "syneeg/protocol.h"

static void refresh_crc(uint8_t frame[SYNEEG_NATIVE_SAMPLE_BYTES]) {
    const uint32_t crc = syneeg_crc32c(frame, 60u);
    frame[60] = (uint8_t)(crc >> 24u);
    frame[61] = (uint8_t)(crc >> 16u);
    frame[62] = (uint8_t)(crc >> 8u);
    frame[63] = (uint8_t)crc;
}

int test_protocol(void) {
    static const uint8_t crc_vector[] = "123456789";
    TEST_ASSERT(syneeg_crc32c(crc_vector, sizeof(crc_vector) - 1u) == UINT32_C(0xE3069283));

    syneeg_sample_t source;
    memset(&source, 0, sizeof(source));
    source.node_id = 0x10203040u;
    source.segment_id = 0x11223344u;
    source.sequence = 0x50607080u;
    source.epoch_tick = UINT64_C(0x0102030405060708);
    source.configuration_generation = 42u;
    source.topology_generation = 0x0A0B0C0Du;
    source.gaps_before = 2u;
    source.clock_quality = 7u;
    source.ads_status[0] = 0xC0u;
    source.ads_status[1] = 0xAAu;
    source.ads_status[2] = 0x55u;
    source.flags = 3u;
    source.channel[0] = -8388608;
    source.channel[1] = -1;
    source.channel[2] = 0;
    source.channel[3] = 1;
    source.channel[4] = 8388607;

    uint8_t encoded[SYNEEG_NATIVE_SAMPLE_BYTES];
    size_t written = 0u;
    TEST_ASSERT(syneeg_protocol_encode_sample(&source, encoded, sizeof(encoded), &written) ==
                SYNEEG_OK);
    TEST_ASSERT(written == SYNEEG_NATIVE_SAMPLE_BYTES);
    uint8_t valid_eeg[SYNEEG_NATIVE_SAMPLE_BYTES];
    memcpy(valid_eeg, encoded, sizeof(valid_eeg));

    syneeg_sample_t decoded;
    TEST_ASSERT(syneeg_protocol_decode_sample(encoded, written, &decoded) == SYNEEG_OK);
    TEST_ASSERT(decoded.node_id == source.node_id);
    TEST_ASSERT(decoded.segment_id == source.segment_id);
    TEST_ASSERT(decoded.sequence == source.sequence);
    TEST_ASSERT(decoded.epoch_tick == source.epoch_tick);
    TEST_ASSERT(decoded.configuration_generation == source.configuration_generation);
    TEST_ASSERT(decoded.topology_generation == source.topology_generation);
    TEST_ASSERT(decoded.gaps_before == source.gaps_before);
    TEST_ASSERT(decoded.clock_quality == source.clock_quality);
    TEST_ASSERT(decoded.channel[0] == -8388608);
    TEST_ASSERT(decoded.channel[1] == -1);
    TEST_ASSERT(decoded.channel[4] == 8388607);

    encoded[10] ^= 0x01u;
    TEST_ASSERT(syneeg_protocol_decode_sample(encoded, written, &decoded) == SYNEEG_ERROR_VERIFY);

    memcpy(encoded, valid_eeg, sizeof(encoded));
    encoded[0] = 2u;
    refresh_crc(encoded);
    TEST_ASSERT(syneeg_protocol_decode_sample(encoded, written, &decoded) == SYNEEG_ERROR_FORMAT);
    memcpy(encoded, valid_eeg, sizeof(encoded));
    encoded[1] = (uint8_t)SYNEEG_FRAME_STATUS;
    refresh_crc(encoded);
    TEST_ASSERT(syneeg_protocol_decode_sample(encoded, written, &decoded) == SYNEEG_ERROR_FORMAT);
    memcpy(encoded, valid_eeg, sizeof(encoded));
    encoded[3] = 23u;
    refresh_crc(encoded);
    TEST_ASSERT(syneeg_protocol_decode_sample(encoded, written, &decoded) == SYNEEG_ERROR_FORMAT);

    uint8_t openbci[SYNEEG_OPENBCI_PACKET_BYTES];
    TEST_ASSERT(syneeg_protocol_encode_openbci(&source, 0u, openbci) == SYNEEG_OK);
    TEST_ASSERT(openbci[0] == 0xA0u);
    TEST_ASSERT(openbci[1] == (uint8_t)source.sequence);
    TEST_ASSERT(openbci[32] == 0xC0u);

    source.channel[7] = 8388608;
    TEST_ASSERT(syneeg_protocol_encode_sample(&source, encoded, sizeof(encoded), &written) ==
                SYNEEG_ERROR_FORMAT);
    TEST_ASSERT(syneeg_protocol_encode_openbci(&source, 0u, openbci) == SYNEEG_ERROR_FORMAT);

    syneeg_optical_result_t optical;
    memset(&optical, 0, sizeof(optical));
    optical.node_id = 0x10203040u;
    optical.segment_id = 9u;
    optical.sequence = 17u;
    optical.carrier_tick = UINT64_C(0x1112131415161718);
    optical.source_id = 3u;
    optical.source_state = SYNEEG_OPTICAL_SOURCE_ACTIVE;
    optical.wavelength_nm = 850u;
    optical.accumulation_count = 4096u;
    optical.detector_id[0] = 10u;
    optical.detector_id[1] = 11u;
    optical.detector_id[2] = 12u;
    optical.detector_id[3] = 13u;
    optical.accumulated_raw[0] = INT32_MIN;
    optical.accumulated_raw[1] = -1;
    optical.accumulated_raw[2] = 0;
    optical.accumulated_raw[3] = INT32_MAX;
    optical.saturation_mask = 0x02u;
    optical.coupling_mask = 0x0Du;
    optical.fault_evidence = 0x04u;
    optical.clock_quality = 6u;
    optical.configuration_generation = 55u;
    optical.gaps_before = 4u;

    uint8_t optical_frame[SYNEEG_NATIVE_OPTICAL_BYTES];
    TEST_ASSERT(syneeg_protocol_encode_optical(
                    &optical, optical_frame, sizeof(optical_frame), &written) == SYNEEG_OK);
    TEST_ASSERT(written == SYNEEG_NATIVE_OPTICAL_BYTES);
    uint8_t valid_optical[SYNEEG_NATIVE_OPTICAL_BYTES];
    memcpy(valid_optical, optical_frame, sizeof(valid_optical));
    syneeg_optical_result_t decoded_optical;
    TEST_ASSERT(syneeg_protocol_decode_optical(optical_frame, written, &decoded_optical) ==
                SYNEEG_OK);
    TEST_ASSERT(decoded_optical.node_id == optical.node_id);
    TEST_ASSERT(decoded_optical.carrier_tick == optical.carrier_tick);
    TEST_ASSERT(decoded_optical.wavelength_nm == 850u);
    TEST_ASSERT(decoded_optical.accumulation_count == 4096u);
    TEST_ASSERT(decoded_optical.detector_id[3] == 13u);
    TEST_ASSERT(decoded_optical.accumulated_raw[0] == INT32_MIN);
    TEST_ASSERT(decoded_optical.accumulated_raw[3] == INT32_MAX);
    TEST_ASSERT(decoded_optical.saturation_mask == 0x02u);
    TEST_ASSERT(decoded_optical.coupling_mask == 0x0Du);
    TEST_ASSERT(decoded_optical.configuration_generation == 55u);
    TEST_ASSERT(decoded_optical.gaps_before == 4u);
    optical_frame[37] ^= 0x80u;
    TEST_ASSERT(syneeg_protocol_decode_optical(optical_frame, written, &decoded_optical) ==
                SYNEEG_ERROR_VERIFY);

    memcpy(optical_frame, valid_optical, sizeof(optical_frame));
    optical_frame[0] = 2u;
    refresh_crc(optical_frame);
    TEST_ASSERT(syneeg_protocol_decode_optical(optical_frame, written, &decoded_optical) ==
                SYNEEG_ERROR_FORMAT);
    memcpy(optical_frame, valid_optical, sizeof(optical_frame));
    optical_frame[1] = (uint8_t)SYNEEG_FRAME_STATUS;
    refresh_crc(optical_frame);
    TEST_ASSERT(syneeg_protocol_decode_optical(optical_frame, written, &decoded_optical) ==
                SYNEEG_ERROR_FORMAT);
    memcpy(optical_frame, valid_optical, sizeof(optical_frame));
    optical_frame[3] = 3u;
    refresh_crc(optical_frame);
    TEST_ASSERT(syneeg_protocol_decode_optical(optical_frame, written, &decoded_optical) ==
                SYNEEG_ERROR_FORMAT);
    memcpy(optical_frame, valid_optical, sizeof(optical_frame));
    optical_frame[26] = 0u;
    optical_frame[27] = 1u;
    refresh_crc(optical_frame);
    TEST_ASSERT(syneeg_protocol_decode_optical(optical_frame, written, &decoded_optical) ==
                SYNEEG_ERROR_FORMAT);
    memcpy(optical_frame, valid_optical, sizeof(optical_frame));
    optical_frame[25] = 0x7Fu;
    refresh_crc(optical_frame);
    TEST_ASSERT(syneeg_protocol_decode_optical(optical_frame, written, &decoded_optical) ==
                SYNEEG_ERROR_FORMAT);
    return 0;
}
