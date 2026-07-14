#ifndef SYNEEG_ADS1299_H
#define SYNEEG_ADS1299_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "syneeg/sample.h"
#include "syneeg/status.h"

#define SYNEEG_ADS1299_FRAME_BYTES 27u
#define SYNEEG_ADS1299_REGISTER_COUNT 24u

typedef enum {
    SYNEEG_ADS1299_REG_ID = 0x00,
    SYNEEG_ADS1299_REG_CONFIG1 = 0x01,
    SYNEEG_ADS1299_REG_CONFIG2 = 0x02,
    SYNEEG_ADS1299_REG_CONFIG3 = 0x03,
    SYNEEG_ADS1299_REG_LOFF = 0x04,
    SYNEEG_ADS1299_REG_CH1SET = 0x05,
    SYNEEG_ADS1299_REG_CH2SET = 0x06,
    SYNEEG_ADS1299_REG_CH3SET = 0x07,
    SYNEEG_ADS1299_REG_CH4SET = 0x08,
    SYNEEG_ADS1299_REG_CH5SET = 0x09,
    SYNEEG_ADS1299_REG_CH6SET = 0x0A,
    SYNEEG_ADS1299_REG_CH7SET = 0x0B,
    SYNEEG_ADS1299_REG_CH8SET = 0x0C,
    SYNEEG_ADS1299_REG_BIAS_SENSP = 0x0D,
    SYNEEG_ADS1299_REG_BIAS_SENSN = 0x0E,
    SYNEEG_ADS1299_REG_LOFF_SENSP = 0x0F,
    SYNEEG_ADS1299_REG_LOFF_SENSN = 0x10,
    SYNEEG_ADS1299_REG_LOFF_FLIP = 0x11,
    SYNEEG_ADS1299_REG_LOFF_STATP = 0x12,
    SYNEEG_ADS1299_REG_LOFF_STATN = 0x13,
    SYNEEG_ADS1299_REG_GPIO = 0x14,
    SYNEEG_ADS1299_REG_MISC1 = 0x15,
    SYNEEG_ADS1299_REG_MISC2 = 0x16,
    SYNEEG_ADS1299_REG_CONFIG4 = 0x17
} syneeg_ads1299_register_t;

typedef enum {
    SYNEEG_ADS1299_CMD_WAKEUP = 0x02,
    SYNEEG_ADS1299_CMD_STANDBY = 0x04,
    SYNEEG_ADS1299_CMD_RESET = 0x06,
    SYNEEG_ADS1299_CMD_START = 0x08,
    SYNEEG_ADS1299_CMD_STOP = 0x0A,
    SYNEEG_ADS1299_CMD_RDATAC = 0x10,
    SYNEEG_ADS1299_CMD_SDATAC = 0x11,
    SYNEEG_ADS1299_CMD_RDATA = 0x12
} syneeg_ads1299_command_t;

typedef struct {
    void *context;
    bool (*select)(void *context, bool asserted);
    bool (*transfer)(void *context, const uint8_t *tx, uint8_t *rx, size_t length);
    bool (*set_reset)(void *context, bool high);
    bool (*set_start)(void *context, bool high);
    void (*delay_us)(void *context, uint32_t microseconds);
} syneeg_ads1299_bus_t;

typedef struct {
    syneeg_ads1299_bus_t bus;
    uint8_t device_id;
    bool continuous;
    bool started;
} syneeg_ads1299_t;

typedef struct {
    uint8_t config1;
    uint8_t config2;
    uint8_t config3;
    uint8_t loff;
    uint8_t channel_set[SYNEEG_CHANNEL_COUNT];
    uint8_t bias_sensp;
    uint8_t bias_sensn;
    uint8_t misc1;
    uint8_t config4;
} syneeg_ads1299_profile_t;

typedef enum {
    SYNEEG_ADS1299_RATE_250_SPS = 250,
    SYNEEG_ADS1299_RATE_500_SPS = 500,
    SYNEEG_ADS1299_RATE_1000_SPS = 1000,
    SYNEEG_ADS1299_RATE_2000_SPS = 2000
} syneeg_ads1299_sample_rate_t;

typedef struct {
    syneeg_ads1299_sample_rate_t sample_rate;
    uint8_t config1_dr_bits;
    bool product_profile;
    bool qualification_target;
} syneeg_ads1299_rate_info_t;

syneeg_status_t syneeg_ads1299_init(syneeg_ads1299_t *device,
                                    const syneeg_ads1299_bus_t *bus);
syneeg_status_t syneeg_ads1299_hardware_reset(syneeg_ads1299_t *device);
syneeg_status_t syneeg_ads1299_command(syneeg_ads1299_t *device,
                                      syneeg_ads1299_command_t command);
syneeg_status_t syneeg_ads1299_read_register(syneeg_ads1299_t *device,
                                             syneeg_ads1299_register_t address,
                                             uint8_t *value);
syneeg_status_t syneeg_ads1299_write_register(syneeg_ads1299_t *device,
                                              syneeg_ads1299_register_t address,
                                              uint8_t value,
                                              bool verify);
syneeg_status_t syneeg_ads1299_apply_profile(syneeg_ads1299_t *device,
                                             const syneeg_ads1299_profile_t *profile);
syneeg_status_t syneeg_ads1299_start(syneeg_ads1299_t *device);
syneeg_status_t syneeg_ads1299_stop(syneeg_ads1299_t *device);
syneeg_status_t syneeg_ads1299_read_frame(syneeg_ads1299_t *device,
                                         syneeg_sample_t *sample);

syneeg_status_t syneeg_ads1299_rate_info(syneeg_ads1299_sample_rate_t sample_rate,
                                         syneeg_ads1299_rate_info_t *info);
syneeg_status_t syneeg_ads1299_profile_8ch(syneeg_ads1299_sample_rate_t sample_rate,
                                           syneeg_ads1299_profile_t *profile);
int32_t syneeg_ads1299_sign_extend24(const uint8_t bytes[3]);
bool syneeg_ads1299_is_eight_channel_id(uint8_t value);

#endif
