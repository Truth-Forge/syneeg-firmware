#include "syneeg/ads1299.h"

#include <string.h>

#define ADS1299_RREG_BASE 0x20u
#define ADS1299_WREG_BASE 0x40u
#define ADS1299_ID_DEVICE_AND_CHANNEL_MASK 0x0Fu
#define ADS1299_ID_EIGHT_CHANNEL_VALUE 0x0Eu
#define ADS1299_CONFIG1_FIXED_BITS 0x90u

static bool bus_is_valid(const syneeg_ads1299_bus_t *bus) {
    return bus != NULL && bus->select != NULL && bus->transfer != NULL &&
           bus->set_reset != NULL && bus->set_start != NULL && bus->delay_us != NULL;
}

static bool command_is_valid(syneeg_ads1299_command_t command) {
    switch (command) {
        case SYNEEG_ADS1299_CMD_WAKEUP:
        case SYNEEG_ADS1299_CMD_STANDBY:
        case SYNEEG_ADS1299_CMD_RESET:
        case SYNEEG_ADS1299_CMD_START:
        case SYNEEG_ADS1299_CMD_STOP:
        case SYNEEG_ADS1299_CMD_RDATAC:
        case SYNEEG_ADS1299_CMD_SDATAC:
        case SYNEEG_ADS1299_CMD_RDATA:
            return true;
        default:
            return false;
    }
}

static bool register_is_valid(syneeg_ads1299_register_t address) {
    const int value = (int)address;
    return value >= (int)SYNEEG_ADS1299_REG_ID &&
           value < (int)SYNEEG_ADS1299_REGISTER_COUNT;
}

static syneeg_status_t transfer_selected(syneeg_ads1299_t *device,
                                         const uint8_t *tx,
                                         uint8_t *rx,
                                         size_t length) {
    if (!device->bus.select(device->bus.context, true)) {
        return SYNEEG_ERROR_IO;
    }
    const bool transferred = device->bus.transfer(device->bus.context, tx, rx, length);
    const bool deselected = device->bus.select(device->bus.context, false);
    return transferred && deselected ? SYNEEG_OK : SYNEEG_ERROR_IO;
}

bool syneeg_ads1299_is_eight_channel_id(uint8_t value) {
    return (value & ADS1299_ID_DEVICE_AND_CHANNEL_MASK) == ADS1299_ID_EIGHT_CHANNEL_VALUE;
}

syneeg_status_t syneeg_ads1299_init(syneeg_ads1299_t *device,
                                    const syneeg_ads1299_bus_t *bus) {
    if (device == NULL || !bus_is_valid(bus)) {
        return SYNEEG_ERROR_ARGUMENT;
    }

    memset(device, 0, sizeof(*device));
    device->bus = *bus;
    return syneeg_ads1299_hardware_reset(device);
}

syneeg_status_t syneeg_ads1299_hardware_reset(syneeg_ads1299_t *device) {
    if (device == NULL || !bus_is_valid(&device->bus)) {
        return SYNEEG_ERROR_ARGUMENT;
    }

    device->started = false;
    device->continuous = false;
    if (!device->bus.set_start(device->bus.context, false) ||
        !device->bus.set_reset(device->bus.context, true)) {
        return SYNEEG_ERROR_IO;
    }
    device->bus.delay_us(device->bus.context, 1000u);
    if (!device->bus.set_reset(device->bus.context, false)) {
        return SYNEEG_ERROR_IO;
    }
    device->bus.delay_us(device->bus.context, 10u);
    if (!device->bus.set_reset(device->bus.context, true)) {
        return SYNEEG_ERROR_IO;
    }
    device->bus.delay_us(device->bus.context, 10000u);

    syneeg_status_t status = syneeg_ads1299_command(device, SYNEEG_ADS1299_CMD_SDATAC);
    if (status != SYNEEG_OK) {
        return status;
    }

    uint8_t id = 0u;
    status = syneeg_ads1299_read_register(device, SYNEEG_ADS1299_REG_ID, &id);
    if (status != SYNEEG_OK) {
        return status;
    }
    device->device_id = id;
    if (!syneeg_ads1299_is_eight_channel_id(id)) {
        return SYNEEG_ERROR_DEVICE_ID;
    }
    return SYNEEG_OK;
}

syneeg_status_t syneeg_ads1299_command(syneeg_ads1299_t *device,
                                      syneeg_ads1299_command_t command) {
    if (device == NULL || !bus_is_valid(&device->bus) || !command_is_valid(command)) {
        return SYNEEG_ERROR_ARGUMENT;
    }
    const uint8_t byte = (uint8_t)command;
    syneeg_status_t status = transfer_selected(device, &byte, NULL, 1u);
    if (status == SYNEEG_OK) {
        device->bus.delay_us(device->bus.context, 4u);
        if (command == SYNEEG_ADS1299_CMD_RDATAC) {
            device->continuous = true;
        } else if (command == SYNEEG_ADS1299_CMD_SDATAC) {
            device->continuous = false;
        }
    }
    return status;
}

syneeg_status_t syneeg_ads1299_read_register(syneeg_ads1299_t *device,
                                             syneeg_ads1299_register_t address,
                                             uint8_t *value) {
    if (device == NULL || value == NULL || !register_is_valid(address)) {
        return SYNEEG_ERROR_ARGUMENT;
    }
    if (device->continuous) {
        return SYNEEG_ERROR_STATE;
    }

    const uint8_t command = (uint8_t)(ADS1299_RREG_BASE | (uint8_t)address);
    const uint8_t count = 0x00u;
    const uint8_t dummy = 0x00u;
    uint8_t result = 0u;
    if (!device->bus.select(device->bus.context, true)) {
        return SYNEEG_ERROR_IO;
    }
    bool ok = device->bus.transfer(device->bus.context, &command, NULL, 1u);
    device->bus.delay_us(device->bus.context, 4u);
    ok = ok && device->bus.transfer(device->bus.context, &count, NULL, 1u);
    device->bus.delay_us(device->bus.context, 4u);
    ok = ok && device->bus.transfer(device->bus.context, &dummy, &result, 1u);
    ok = device->bus.select(device->bus.context, false) && ok;
    if (!ok) {
        return SYNEEG_ERROR_IO;
    }
    *value = result;
    return SYNEEG_OK;
}

syneeg_status_t syneeg_ads1299_write_register(syneeg_ads1299_t *device,
                                              syneeg_ads1299_register_t address,
                                              uint8_t value,
                                              bool verify) {
    if (device == NULL || !register_is_valid(address) ||
        address == SYNEEG_ADS1299_REG_ID ||
        address == SYNEEG_ADS1299_REG_LOFF_STATP ||
        address == SYNEEG_ADS1299_REG_LOFF_STATN) {
        return SYNEEG_ERROR_ARGUMENT;
    }
    if (device->continuous) {
        return SYNEEG_ERROR_STATE;
    }

    const uint8_t command = (uint8_t)(ADS1299_WREG_BASE | (uint8_t)address);
    const uint8_t count = 0x00u;
    if (!device->bus.select(device->bus.context, true)) {
        return SYNEEG_ERROR_IO;
    }
    bool ok = device->bus.transfer(device->bus.context, &command, NULL, 1u);
    device->bus.delay_us(device->bus.context, 4u);
    ok = ok && device->bus.transfer(device->bus.context, &count, NULL, 1u);
    device->bus.delay_us(device->bus.context, 4u);
    ok = ok && device->bus.transfer(device->bus.context, &value, NULL, 1u);
    ok = device->bus.select(device->bus.context, false) && ok;
    syneeg_status_t status = ok ? SYNEEG_OK : SYNEEG_ERROR_IO;
    if (status != SYNEEG_OK || !verify) {
        return status;
    }

    uint8_t readback = 0u;
    status = syneeg_ads1299_read_register(device, address, &readback);
    if (status != SYNEEG_OK) {
        return status;
    }
    return readback == value ? SYNEEG_OK : SYNEEG_ERROR_VERIFY;
}

syneeg_status_t syneeg_ads1299_rate_info(syneeg_ads1299_sample_rate_t sample_rate,
                                         syneeg_ads1299_rate_info_t *info) {
    if (info == NULL) {
        return SYNEEG_ERROR_ARGUMENT;
    }

    memset(info, 0, sizeof(*info));
    info->sample_rate = sample_rate;
    switch (sample_rate) {
        case SYNEEG_ADS1299_RATE_250_SPS:
            info->config1_dr_bits = 0x06u;
            info->product_profile = true;
            break;
        case SYNEEG_ADS1299_RATE_500_SPS:
            info->config1_dr_bits = 0x05u;
            info->product_profile = true;
            break;
        case SYNEEG_ADS1299_RATE_1000_SPS:
            info->config1_dr_bits = 0x04u;
            info->product_profile = true;
            break;
        case SYNEEG_ADS1299_RATE_2000_SPS:
            info->config1_dr_bits = 0x03u;
            info->qualification_target = true;
            break;
        default:
            return SYNEEG_ERROR_ARGUMENT;
    }
    return SYNEEG_OK;
}

syneeg_status_t syneeg_ads1299_profile_8ch(syneeg_ads1299_sample_rate_t sample_rate,
                                           syneeg_ads1299_profile_t *profile) {
    if (profile == NULL) {
        return SYNEEG_ERROR_ARGUMENT;
    }
    syneeg_ads1299_rate_info_t info;
    syneeg_status_t status = syneeg_ads1299_rate_info(sample_rate, &info);
    if (status != SYNEEG_OK) {
        return status;
    }

    memset(profile, 0, sizeof(*profile));

    profile->config1 = (uint8_t)(ADS1299_CONFIG1_FIXED_BITS | info.config1_dr_bits);
    profile->config2 = 0xC0u;
    profile->config3 = 0xECu;
    profile->loff = 0x02u;
    for (size_t i = 0u; i < SYNEEG_CHANNEL_COUNT; ++i) {
        profile->channel_set[i] = 0x68u;
    }
    profile->bias_sensp = 0xFFu;
    profile->bias_sensn = 0xFFu;
    profile->misc1 = 0x00u;
    profile->config4 = 0x00u;
    return SYNEEG_OK;
}

syneeg_status_t syneeg_ads1299_apply_profile(syneeg_ads1299_t *device,
                                             const syneeg_ads1299_profile_t *profile) {
    if (device == NULL || profile == NULL || device->started) {
        return SYNEEG_ERROR_ARGUMENT;
    }

    const struct {
        syneeg_ads1299_register_t address;
        uint8_t value;
    } fixed[] = {
        {SYNEEG_ADS1299_REG_CONFIG1, profile->config1},
        {SYNEEG_ADS1299_REG_CONFIG2, profile->config2},
        {SYNEEG_ADS1299_REG_CONFIG3, profile->config3},
        {SYNEEG_ADS1299_REG_LOFF, profile->loff}
    };

    for (size_t i = 0u; i < sizeof(fixed) / sizeof(fixed[0]); ++i) {
        syneeg_status_t status = syneeg_ads1299_write_register(
            device, fixed[i].address, fixed[i].value, true);
        if (status != SYNEEG_OK) {
            return status;
        }
    }
    for (size_t i = 0u; i < SYNEEG_CHANNEL_COUNT; ++i) {
        const syneeg_ads1299_register_t address =
            (syneeg_ads1299_register_t)((uint8_t)SYNEEG_ADS1299_REG_CH1SET + (uint8_t)i);
        syneeg_status_t status = syneeg_ads1299_write_register(
            device, address, profile->channel_set[i], true);
        if (status != SYNEEG_OK) {
            return status;
        }
    }

    const struct {
        syneeg_ads1299_register_t address;
        uint8_t value;
    } remaining[] = {
        {SYNEEG_ADS1299_REG_BIAS_SENSP, profile->bias_sensp},
        {SYNEEG_ADS1299_REG_BIAS_SENSN, profile->bias_sensn},
        {SYNEEG_ADS1299_REG_MISC1, profile->misc1},
        {SYNEEG_ADS1299_REG_CONFIG4, profile->config4}
    };
    for (size_t i = 0u; i < sizeof(remaining) / sizeof(remaining[0]); ++i) {
        syneeg_status_t status = syneeg_ads1299_write_register(
            device, remaining[i].address, remaining[i].value, true);
        if (status != SYNEEG_OK) {
            return status;
        }
    }
    device->bus.delay_us(device->bus.context, 200000u);
    return SYNEEG_OK;
}

syneeg_status_t syneeg_ads1299_start(syneeg_ads1299_t *device) {
    if (device == NULL || device->started) {
        return SYNEEG_ERROR_STATE;
    }
    if (!device->bus.set_start(device->bus.context, true)) {
        return SYNEEG_ERROR_IO;
    }
    syneeg_status_t status = syneeg_ads1299_command(device, SYNEEG_ADS1299_CMD_RDATAC);
    if (status != SYNEEG_OK) {
        (void)device->bus.set_start(device->bus.context, false);
        return status;
    }
    device->started = true;
    return SYNEEG_OK;
}

syneeg_status_t syneeg_ads1299_stop(syneeg_ads1299_t *device) {
    if (device == NULL) {
        return SYNEEG_ERROR_ARGUMENT;
    }
    if (!device->bus.set_start(device->bus.context, false)) {
        return SYNEEG_ERROR_IO;
    }
    syneeg_status_t status = syneeg_ads1299_command(device, SYNEEG_ADS1299_CMD_SDATAC);
    device->started = false;
    return status;
}

int32_t syneeg_ads1299_sign_extend24(const uint8_t bytes[3]) {
    uint32_t value = ((uint32_t)bytes[0] << 16u) |
                     ((uint32_t)bytes[1] << 8u) |
                     (uint32_t)bytes[2];
    if ((value & 0x00800000u) != 0u) {
        value |= 0xFF000000u;
    }
    return (int32_t)value;
}

syneeg_status_t syneeg_ads1299_read_frame(syneeg_ads1299_t *device,
                                         syneeg_sample_t *sample) {
    if (device == NULL || sample == NULL) {
        return SYNEEG_ERROR_ARGUMENT;
    }
    if (!device->started || !device->continuous) {
        return SYNEEG_ERROR_STATE;
    }

    uint8_t tx[SYNEEG_ADS1299_FRAME_BYTES] = {0u};
    uint8_t rx[SYNEEG_ADS1299_FRAME_BYTES] = {0u};
    syneeg_status_t status = transfer_selected(device, tx, rx, sizeof(rx));
    if (status != SYNEEG_OK) {
        return status;
    }
    if ((rx[0] & 0xF0u) != 0xC0u) {
        return SYNEEG_ERROR_FORMAT;
    }

    memcpy(sample->ads_status, rx, 3u);
    for (size_t i = 0u; i < SYNEEG_CHANNEL_COUNT; ++i) {
        sample->channel[i] = syneeg_ads1299_sign_extend24(&rx[3u + (i * 3u)]);
    }
    return SYNEEG_OK;
}
