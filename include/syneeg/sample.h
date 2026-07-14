#ifndef SYNEEG_SAMPLE_H
#define SYNEEG_SAMPLE_H

#include <stdint.h>

#define SYNEEG_CHANNEL_COUNT 8u

typedef struct {
    uint8_t ads_status[3];
    int32_t channel[SYNEEG_CHANNEL_COUNT];
    uint32_t sequence;
    uint32_t node_id;
    uint32_t segment_id;
    uint32_t membership_generation;
    uint64_t epoch_tick;
    uint16_t configuration_generation;
    uint16_t gaps_before;
    uint8_t clock_quality;
    uint8_t flags;
} syneeg_sample_t;

#endif
