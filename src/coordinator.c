#include "syneeg/coordinator.h"

#include <string.h>

void syneeg_coordinator_init(syneeg_coordinator_t *coordinator, uint32_t self_id) {
    if (coordinator == NULL) {
        return;
    }
    memset(coordinator, 0, sizeof(*coordinator));
    coordinator->self_id = self_id;
    coordinator->leader_id = self_id;
    coordinator->state = SYNEEG_COORDINATOR_SOLO;
}

syneeg_status_t syneeg_coordinator_observe(syneeg_coordinator_t *coordinator,
                                           uint32_t node_id,
                                           uint32_t now_ms,
                                           bool eligible) {
    if (coordinator == NULL || node_id == 0u || node_id == coordinator->self_id) {
        return SYNEEG_ERROR_ARGUMENT;
    }
    for (size_t i = 0u; i < coordinator->peer_count; ++i) {
        if (coordinator->peers[i].node_id == node_id) {
            if (coordinator->peers[i].eligible != eligible ||
                !coordinator->peers[i].counted_live) {
                ++coordinator->membership_revision;
                coordinator->membership_dirty = true;
                coordinator->state = SYNEEG_COORDINATOR_SEGMENT_REQUIRED;
            }
            coordinator->peers[i].last_seen_ms = now_ms;
            coordinator->peers[i].eligible = eligible;
            coordinator->peers[i].counted_live = eligible;
            return SYNEEG_OK;
        }
    }
    if (coordinator->peer_count >= SYNEEG_MAX_ACTIVE_NODES - 1u) {
        return SYNEEG_ERROR_CAPACITY;
    }
    syneeg_peer_t *peer = &coordinator->peers[coordinator->peer_count++];
    peer->node_id = node_id;
    peer->last_seen_ms = now_ms;
    peer->eligible = eligible;
    peer->counted_live = eligible;
    ++coordinator->membership_revision;
    coordinator->membership_dirty = true;
    coordinator->state = SYNEEG_COORDINATOR_SEGMENT_REQUIRED;
    return SYNEEG_OK;
}

syneeg_status_t syneeg_coordinator_elect(syneeg_coordinator_t *coordinator,
                                         uint32_t now_ms,
                                         uint32_t peer_timeout_ms) {
    if (coordinator == NULL || peer_timeout_ms == 0u) {
        return SYNEEG_ERROR_ARGUMENT;
    }

    uint32_t candidate = coordinator->self_id;
    size_t live_peers = 0u;
    for (size_t i = 0u; i < coordinator->peer_count; ++i) {
        syneeg_peer_t *peer = &coordinator->peers[i];
        const uint32_t age = now_ms - peer->last_seen_ms;
        const bool live = peer->eligible && age <= peer_timeout_ms;
        if (peer->counted_live != live) {
            peer->counted_live = live;
            ++coordinator->membership_revision;
            coordinator->membership_dirty = true;
        }
        if (!live) {
            continue;
        }
        ++live_peers;
        if (peer->node_id < candidate) {
            candidate = peer->node_id;
        }
    }

    const bool leader_changed = coordinator->leader_id != candidate;
    coordinator->leader_id = candidate;
    if (coordinator->segment_id != 0u &&
        (leader_changed || coordinator->membership_dirty ||
         coordinator->state == SYNEEG_COORDINATOR_SEGMENT_REQUIRED)) {
        coordinator->state = SYNEEG_COORDINATOR_SEGMENT_REQUIRED;
        return SYNEEG_OK;
    }

    if (live_peers == 0u) {
        coordinator->state = SYNEEG_COORDINATOR_SOLO;
    } else if (candidate == coordinator->self_id) {
        coordinator->state = SYNEEG_COORDINATOR_LEADER;
    } else {
        coordinator->state = SYNEEG_COORDINATOR_FOLLOWER;
    }
    return SYNEEG_OK;
}

void syneeg_coordinator_commit_epoch(syneeg_coordinator_t *coordinator,
                                     uint32_t segment_id) {
    if (coordinator == NULL || segment_id == 0u) {
        return;
    }
    coordinator->segment_id = segment_id;
    coordinator->membership_dirty = false;
    coordinator->state = coordinator->leader_id == coordinator->self_id
                             ? SYNEEG_COORDINATOR_LEADER
                             : SYNEEG_COORDINATOR_FOLLOWER;
}

void syneeg_coordinator_require_segment(syneeg_coordinator_t *coordinator) {
    if (coordinator != NULL) {
        coordinator->state = SYNEEG_COORDINATOR_SEGMENT_REQUIRED;
    }
}
