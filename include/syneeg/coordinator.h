#ifndef SYNEEG_COORDINATOR_H
#define SYNEEG_COORDINATOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "syneeg/status.h"

#define SYNEEG_MAX_ACTIVE_NODES 8u

typedef enum {
    SYNEEG_COORDINATOR_SOLO = 0,
    SYNEEG_COORDINATOR_DISCOVERING,
    SYNEEG_COORDINATOR_FOLLOWER,
    SYNEEG_COORDINATOR_LEADER,
    SYNEEG_COORDINATOR_SEGMENT_REQUIRED
} syneeg_coordinator_state_t;

typedef struct {
    uint32_t node_id;
    uint32_t last_seen_ms;
    bool eligible;
    bool counted_live;
} syneeg_peer_t;

typedef struct {
    uint32_t self_id;
    uint32_t leader_id;
    uint32_t segment_id;
    uint32_t membership_revision;
    syneeg_coordinator_state_t state;
    syneeg_peer_t peers[SYNEEG_MAX_ACTIVE_NODES - 1u];
    size_t peer_count;
    bool membership_dirty;
} syneeg_coordinator_t;

void syneeg_coordinator_init(syneeg_coordinator_t *coordinator, uint32_t self_id);
syneeg_status_t syneeg_coordinator_observe(syneeg_coordinator_t *coordinator,
                                           uint32_t node_id,
                                           uint32_t now_ms,
                                           bool eligible);
syneeg_status_t syneeg_coordinator_elect(syneeg_coordinator_t *coordinator,
                                         uint32_t now_ms,
                                         uint32_t peer_timeout_ms);
void syneeg_coordinator_commit_epoch(syneeg_coordinator_t *coordinator,
                                     uint32_t segment_id);
void syneeg_coordinator_require_segment(syneeg_coordinator_t *coordinator);

#endif
