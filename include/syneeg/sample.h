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
    uint32_t topology_generation;
    uint64_t epoch_tick;
    uint16_t configuration_generation;
    uint16_t gaps_before;
    uint8_t clock_quality;
    uint8_t flags;
} syneeg_sample_t;

#define SYNEEG_OPTICAL_DETECTOR_COUNT 4u

typedef enum {
    SYNEEG_OPTICAL_SOURCE_DISABLED = 0,
    SYNEEG_OPTICAL_SOURCE_ACTIVE = 1,
    SYNEEG_OPTICAL_SOURCE_FAULT = 2
} syneeg_optical_source_state_t;

/* Reduced optical result emitted by the local optical processor. Raw ADC
 * samples do not cross the SynPod-to-SynDock boundary. */
typedef struct {
    uint32_t node_id;
    uint32_t segment_id;
    uint32_t sequence;
    uint64_t carrier_tick;
    uint32_t accumulation_count;
    int32_t accumulated_raw[SYNEEG_OPTICAL_DETECTOR_COUNT];
    uint16_t wavelength_nm;
    uint16_t configuration_generation;
    uint16_t gaps_before;
    uint8_t source_id;
    uint8_t source_state;
    uint8_t detector_id[SYNEEG_OPTICAL_DETECTOR_COUNT];
    uint8_t saturation_mask;
    uint8_t coupling_mask;
    uint8_t fault_evidence;
    uint8_t clock_quality;
} syneeg_optical_result_t;

#endif
