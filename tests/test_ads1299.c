#include "test.h"

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>
#include <string.h>

#include "syneeg/ads1299.h"

typedef struct {
    uint8_t registers[SYNEEG_ADS1299_REGISTER_COUNT];
    uint8_t frame[SYNEEG_ADS1299_FRAME_BYTES];
    bool selected;
    bool reset_high;
    bool start_high;
    uint8_t pending_address;
    bool pending_read;
    bool pending_write;
    bool pending_count;
    uint32_t maximum_delay_us;
} mock_ads_t;

static bool mock_select(void *context, bool asserted) {
    ((mock_ads_t *)context)->selected = asserted;
    return true;
}

static bool mock_transfer(void *context, const uint8_t *tx, uint8_t *rx, size_t length) {
    mock_ads_t *mock = (mock_ads_t *)context;
    if (!mock->selected || tx == NULL) {
        return false;
    }
    if (length == 1u && !mock->pending_read && !mock->pending_write &&
        (tx[0] & 0xE0u) == 0x20u) {
        mock->pending_address = tx[0] & 0x1Fu;
        mock->pending_read = true;
        mock->pending_count = true;
        return true;
    }
    if (length == 1u && !mock->pending_read && !mock->pending_write &&
        (tx[0] & 0xE0u) == 0x40u) {
        mock->pending_address = tx[0] & 0x1Fu;
        mock->pending_write = true;
        mock->pending_count = true;
        return true;
    }
    if (length == 1u && mock->pending_count) {
        mock->pending_count = false;
        return true;
    }
    if (length == 1u && mock->pending_read) {
        if (rx != NULL) {
            rx[0] = mock->registers[mock->pending_address];
        }
        mock->pending_read = false;
        return true;
    }
    if (length == 1u && mock->pending_write) {
        mock->registers[mock->pending_address] = tx[0];
        mock->pending_write = false;
        return true;
    }
    if (length == SYNEEG_ADS1299_FRAME_BYTES && rx != NULL) {
        memcpy(rx, mock->frame, length);
        return true;
    }
    return length == 1u;
}

static bool mock_reset(void *context, bool high) {
    ((mock_ads_t *)context)->reset_high = high;
    return true;
}

static bool mock_start(void *context, bool high) {
    ((mock_ads_t *)context)->start_high = high;
    return true;
}

static void mock_delay(void *context, uint32_t microseconds) {
    mock_ads_t *mock = (mock_ads_t *)context;
    if (microseconds > mock->maximum_delay_us) {
        mock->maximum_delay_us = microseconds;
    }
}

int test_ads1299(void) {
    const uint8_t positive[3] = {0x7Fu, 0xFFu, 0xFFu};
    const uint8_t negative[3] = {0x80u, 0x00u, 0x00u};
    const uint8_t minus_one[3] = {0xFFu, 0xFFu, 0xFFu};
    TEST_ASSERT(syneeg_ads1299_sign_extend24(positive) == 8388607);
    TEST_ASSERT(syneeg_ads1299_sign_extend24(negative) == -8388608);
    TEST_ASSERT(syneeg_ads1299_sign_extend24(minus_one) == -1);
    TEST_ASSERT(syneeg_ads1299_is_eight_channel_id(0x3Eu));
    TEST_ASSERT(syneeg_ads1299_is_eight_channel_id(0x5Eu));
    TEST_ASSERT(!syneeg_ads1299_is_eight_channel_id(0x3Du));

    mock_ads_t mock;
    memset(&mock, 0, sizeof(mock));
    mock.registers[SYNEEG_ADS1299_REG_ID] = 0x3Eu;
    mock.frame[0] = 0xC0u;
    mock.frame[1] = 0x12u;
    mock.frame[2] = 0x34u;
    for (size_t i = 0u; i < SYNEEG_CHANNEL_COUNT; ++i) {
        mock.frame[3u + i * 3u] = (uint8_t)i;
        mock.frame[4u + i * 3u] = 0x01u;
        mock.frame[5u + i * 3u] = 0x02u;
    }

    syneeg_ads1299_bus_t bus = {
        .context = &mock,
        .select = mock_select,
        .transfer = mock_transfer,
        .set_reset = mock_reset,
        .set_start = mock_start,
        .delay_us = mock_delay
    };
    syneeg_ads1299_t device;
    TEST_ASSERT(syneeg_ads1299_init(&device, &bus) == SYNEEG_OK);
    TEST_ASSERT(device.device_id == 0x3Eu);

    const syneeg_ads1299_profile_t profile = syneeg_ads1299_profile_8ch_250sps();
    TEST_ASSERT(profile.config1 == 0x96u);
    TEST_ASSERT(profile.config3 == 0xECu);
    TEST_ASSERT(profile.bias_sensp == 0xFFu);
    TEST_ASSERT(profile.bias_sensn == 0xFFu);
    TEST_ASSERT(profile.misc1 == 0x00u);
    TEST_ASSERT(syneeg_ads1299_apply_profile(&device, &profile) == SYNEEG_OK);
    TEST_ASSERT(mock.registers[SYNEEG_ADS1299_REG_CH8SET] == 0x68u);
    TEST_ASSERT(mock.maximum_delay_us >= 200000u);

    TEST_ASSERT(syneeg_ads1299_start(&device) == SYNEEG_OK);
    syneeg_sample_t sample;
    memset(&sample, 0, sizeof(sample));
    TEST_ASSERT(syneeg_ads1299_read_frame(&device, &sample) == SYNEEG_OK);
    TEST_ASSERT(sample.ads_status[1] == 0x12u);
    TEST_ASSERT(sample.channel[0] == 0x000102);
    TEST_ASSERT(sample.channel[7] == 0x070102);
    TEST_ASSERT(syneeg_ads1299_hardware_reset(&device) == SYNEEG_OK);
    TEST_ASSERT(!device.started);
    TEST_ASSERT(!device.continuous);
    TEST_ASSERT(syneeg_ads1299_apply_profile(&device, &profile) == SYNEEG_OK);
    return 0;
}
