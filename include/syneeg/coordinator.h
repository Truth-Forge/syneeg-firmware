#ifndef SYNEEG_COORDINATOR_H
#define SYNEEG_COORDINATOR_H

#include <stdbool.h>
#include <stddef.h>
#include <stdint.h>

#include "syneeg/status.h"

#define SYNEEG_MAX_ACTIVE_NODES 8u
#define SYNEEG_ADDRESS_BROADCAST 0xFFu

/* Command, topology, and segment serials reserve zero and use 32-bit
 * half-range ordering. Exact duplicates are stale; max advances to one. */

typedef enum {
    SYNEEG_DOCK_IDLE = 0,
    SYNEEG_DOCK_SEGMENT_REQUIRED,
    SYNEEG_DOCK_STOP_REQUIRED,
    SYNEEG_DOCK_RUNNING,
    SYNEEG_DOCK_FAULT
} syneeg_dock_state_t;

typedef struct {
    uint32_t node_id;
    uint32_t last_seen_ms;
    uint8_t address;
    bool active;
} syneeg_addressed_node_t;

typedef struct {
    uint32_t authority_id;
    uint32_t segment_id;
    uint32_t last_segment_id;
    uint32_t topology_generation;
    uint32_t active_topology_generation;
    uint32_t next_command_sequence;
    syneeg_dock_state_t state;
    syneeg_addressed_node_t nodes[SYNEEG_MAX_ACTIVE_NODES];
    size_t node_count;
    size_t active_node_count;
} syneeg_dock_coordinator_t;

typedef enum {
    SYNEEG_CONTROL_TOPOLOGY = 1,
    SYNEEG_CONTROL_START = 2,
    SYNEEG_CONTROL_STOP = 3
} syneeg_control_type_t;

typedef struct {
    uint32_t authority_id;
    uint32_t segment_id;
    uint32_t topology_generation;
    uint32_t sequence;
    uint8_t destination_address;
    syneeg_control_type_t type;
} syneeg_control_command_t;

typedef enum {
    SYNEEG_POD_UNBOUND = 0,
    SYNEEG_POD_IDLE,
    SYNEEG_POD_RUNNING,
    SYNEEG_POD_FAULT
} syneeg_pod_state_t;

typedef struct {
    uint32_t node_id;
    uint32_t authority_id;
    uint32_t segment_id;
    uint32_t last_segment_id;
    uint32_t topology_generation;
    uint32_t last_command_sequence;
    uint8_t address;
    syneeg_pod_state_t state;
} syneeg_pod_node_t;

syneeg_status_t syneeg_dock_coordinator_init(syneeg_dock_coordinator_t *coordinator,
                                              uint32_t authority_id);
syneeg_status_t syneeg_dock_coordinator_observe(syneeg_dock_coordinator_t *coordinator,
                                                 uint32_t node_id,
                                                 uint8_t address,
                                                 uint32_t now_ms);
syneeg_status_t syneeg_dock_coordinator_expire(syneeg_dock_coordinator_t *coordinator,
                                                uint32_t now_ms,
                                                uint32_t node_timeout_ms);
syneeg_status_t syneeg_dock_coordinator_start_segment(
    syneeg_dock_coordinator_t *coordinator, uint32_t segment_id);
syneeg_status_t syneeg_dock_coordinator_stop_segment(
    syneeg_dock_coordinator_t *coordinator, syneeg_control_command_t *command);
syneeg_status_t syneeg_dock_coordinator_abort_segment(
    syneeg_dock_coordinator_t *coordinator, syneeg_control_command_t *command);
syneeg_status_t syneeg_dock_coordinator_make_command(
    syneeg_dock_coordinator_t *coordinator,
    uint8_t destination_address,
    syneeg_control_type_t type,
    syneeg_control_command_t *command);
void syneeg_pod_node_init(syneeg_pod_node_t *node,
                          uint32_t node_id,
                          uint8_t address,
                          uint32_t authority_id);
syneeg_status_t syneeg_pod_node_apply(syneeg_pod_node_t *node,
                                      const syneeg_control_command_t *command);

#endif
