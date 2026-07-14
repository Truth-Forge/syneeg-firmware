#include "test.h"

#include <stdint.h>
#include <string.h>

#include "syneeg/protocol.h"

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
    source.membership_generation = 0x0A0B0C0Du;
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

    syneeg_sample_t decoded;
    TEST_ASSERT(syneeg_protocol_decode_sample(encoded, written, &decoded) == SYNEEG_OK);
    TEST_ASSERT(decoded.node_id == source.node_id);
    TEST_ASSERT(decoded.segment_id == source.segment_id);
    TEST_ASSERT(decoded.sequence == source.sequence);
    TEST_ASSERT(decoded.epoch_tick == source.epoch_tick);
    TEST_ASSERT(decoded.configuration_generation == source.configuration_generation);
    TEST_ASSERT(decoded.membership_generation == source.membership_generation);
    TEST_ASSERT(decoded.gaps_before == source.gaps_before);
    TEST_ASSERT(decoded.clock_quality == source.clock_quality);
    TEST_ASSERT(decoded.channel[0] == -8388608);
    TEST_ASSERT(decoded.channel[1] == -1);
    TEST_ASSERT(decoded.channel[4] == 8388607);

    encoded[10] ^= 0x01u;
    TEST_ASSERT(syneeg_protocol_decode_sample(encoded, written, &decoded) == SYNEEG_ERROR_VERIFY);

    uint8_t openbci[SYNEEG_OPENBCI_PACKET_BYTES];
    TEST_ASSERT(syneeg_protocol_encode_openbci(&source, 0u, openbci) == SYNEEG_OK);
    TEST_ASSERT(openbci[0] == 0xA0u);
    TEST_ASSERT(openbci[1] == (uint8_t)source.sequence);
    TEST_ASSERT(openbci[32] == 0xC0u);

    source.channel[7] = 8388608;
    TEST_ASSERT(syneeg_protocol_encode_sample(&source, encoded, sizeof(encoded), &written) ==
                SYNEEG_ERROR_FORMAT);
    TEST_ASSERT(syneeg_protocol_encode_openbci(&source, 0u, openbci) == SYNEEG_ERROR_FORMAT);
    return 0;
}
