#include "syneeg/coordinator.h"

#include <string.h>

#define SERIAL_HALF_RANGE UINT32_C(0x80000000)

static bool address_is_valid(uint8_t address) {
    return address != 0u && address != SYNEEG_ADDRESS_BROADCAST;
}

static uint32_t serial_next(uint32_t value) {
    ++value;
    return value == 0u ? 1u : value;
}

static bool serial_is_newer(uint32_t candidate, uint32_t reference) {
    if (candidate == 0u) {
        return false;
    }
    if (reference == 0u) {
        return true;
    }
    const uint32_t distance = candidate - reference;
    return distance != 0u && distance < SERIAL_HALF_RANGE;
}

static void topology_changed(syneeg_dock_coordinator_t *coordinator) {
    coordinator->topology_generation = serial_next(coordinator->topology_generation);
    if (coordinator->state == SYNEEG_DOCK_RUNNING ||
        coordinator->state == SYNEEG_DOCK_STOP_REQUIRED) {
        coordinator->state = SYNEEG_DOCK_STOP_REQUIRED;
    } else {
        coordinator->state = SYNEEG_DOCK_SEGMENT_REQUIRED;
    }
}

static void populate_command(syneeg_dock_coordinator_t *coordinator,
                             uint8_t destination_address,
                             syneeg_control_type_t type,
                             syneeg_control_command_t *command) {
    coordinator->next_command_sequence = serial_next(coordinator->next_command_sequence);
    command->authority_id = coordinator->authority_id;
    command->segment_id = coordinator->segment_id;
    command->topology_generation = type == SYNEEG_CONTROL_STOP
                                       ? coordinator->active_topology_generation
                                       : coordinator->topology_generation;
    command->sequence = coordinator->next_command_sequence;
    command->destination_address = destination_address;
    command->type = type;
}

syneeg_status_t syneeg_dock_coordinator_init(syneeg_dock_coordinator_t *coordinator,
                                              uint32_t authority_id) {
    if (coordinator == NULL || authority_id == 0u) {
        return SYNEEG_ERROR_ARGUMENT;
    }
    memset(coordinator, 0, sizeof(*coordinator));
    coordinator->authority_id = authority_id;
    coordinator->state = SYNEEG_DOCK_IDLE;
    return SYNEEG_OK;
}

syneeg_status_t syneeg_dock_coordinator_observe(syneeg_dock_coordinator_t *coordinator,
                                                 uint32_t node_id,
                                                 uint8_t address,
                                                 uint32_t now_ms) {
    if (coordinator == NULL || node_id == 0u || !address_is_valid(address)) {
        return SYNEEG_ERROR_ARGUMENT;
    }

    size_t matching_index = SYNEEG_MAX_ACTIVE_NODES;
    size_t reusable_index = SYNEEG_MAX_ACTIVE_NODES;
    for (size_t i = 0u; i < coordinator->node_count; ++i) {
        const syneeg_addressed_node_t *node = &coordinator->nodes[i];
        if (node->node_id == node_id) {
            matching_index = i;
        } else if (node->active && node->address == address) {
            return SYNEEG_ERROR_ADDRESS;
        }
        if (!node->active && reusable_index == SYNEEG_MAX_ACTIVE_NODES) {
            reusable_index = i;
        }
    }

    if (matching_index != SYNEEG_MAX_ACTIVE_NODES) {
        syneeg_addressed_node_t *node = &coordinator->nodes[matching_index];
        const bool changed = !node->active || node->address != address;
        node->last_seen_ms = now_ms;
        node->address = address;
        if (!node->active) {
            node->active = true;
            ++coordinator->active_node_count;
        }
        if (changed) {
            topology_changed(coordinator);
        }
        return SYNEEG_OK;
    }

    size_t insertion_index = reusable_index;
    if (insertion_index == SYNEEG_MAX_ACTIVE_NODES) {
        if (coordinator->node_count >= SYNEEG_MAX_ACTIVE_NODES) {
            return SYNEEG_ERROR_CAPACITY;
        }
        insertion_index = coordinator->node_count;
        ++coordinator->node_count;
    }
    syneeg_addressed_node_t *node = &coordinator->nodes[insertion_index];
    memset(node, 0, sizeof(*node));
    ++coordinator->active_node_count;
    node->node_id = node_id;
    node->last_seen_ms = now_ms;
    node->address = address;
    node->active = true;
    topology_changed(coordinator);
    return SYNEEG_OK;
}

syneeg_status_t syneeg_dock_coordinator_expire(syneeg_dock_coordinator_t *coordinator,
                                                uint32_t now_ms,
                                                uint32_t node_timeout_ms) {
    if (coordinator == NULL || node_timeout_ms == 0u) {
        return SYNEEG_ERROR_ARGUMENT;
    }
    bool changed = false;
    for (size_t i = 0u; i < coordinator->node_count; ++i) {
        syneeg_addressed_node_t *node = &coordinator->nodes[i];
        if (!node->active || now_ms - node->last_seen_ms <= node_timeout_ms) {
            continue;
        }
        node->active = false;
        --coordinator->active_node_count;
        changed = true;
    }
    if (changed) {
        topology_changed(coordinator);
    }
    return SYNEEG_OK;
}

syneeg_status_t syneeg_dock_coordinator_start_segment(
    syneeg_dock_coordinator_t *coordinator, uint32_t segment_id) {
    if (coordinator == NULL || segment_id == 0u || coordinator->active_node_count == 0u) {
        return SYNEEG_ERROR_ARGUMENT;
    }
    if (!serial_is_newer(segment_id, coordinator->last_segment_id)) {
        return SYNEEG_ERROR_STALE;
    }
    if (coordinator->state != SYNEEG_DOCK_SEGMENT_REQUIRED &&
        coordinator->state != SYNEEG_DOCK_IDLE) {
        return SYNEEG_ERROR_STATE;
    }
    coordinator->segment_id = segment_id;
    coordinator->last_segment_id = segment_id;
    coordinator->active_topology_generation = coordinator->topology_generation;
    coordinator->state = SYNEEG_DOCK_RUNNING;
    return SYNEEG_OK;
}

syneeg_status_t syneeg_dock_coordinator_make_command(
    syneeg_dock_coordinator_t *coordinator,
    uint8_t destination_address,
    syneeg_control_type_t type,
    syneeg_control_command_t *command) {
    if (coordinator == NULL || command == NULL ||
        (destination_address != SYNEEG_ADDRESS_BROADCAST &&
         !address_is_valid(destination_address))) {
        return SYNEEG_ERROR_ARGUMENT;
    }
    if (type == SYNEEG_CONTROL_STOP) {
        return SYNEEG_ERROR_STATE;
    }
    if (type != SYNEEG_CONTROL_TOPOLOGY && type != SYNEEG_CONTROL_START) {
        return SYNEEG_ERROR_ARGUMENT;
    }
    if (type == SYNEEG_CONTROL_START && coordinator->state != SYNEEG_DOCK_RUNNING) {
        return SYNEEG_ERROR_STATE;
    }
    if (type == SYNEEG_CONTROL_TOPOLOGY &&
        coordinator->state != SYNEEG_DOCK_SEGMENT_REQUIRED &&
        coordinator->state != SYNEEG_DOCK_IDLE) {
        return SYNEEG_ERROR_STATE;
    }
    populate_command(coordinator, destination_address, type, command);
    return SYNEEG_OK;
}

syneeg_status_t syneeg_dock_coordinator_stop_segment(
    syneeg_dock_coordinator_t *coordinator, syneeg_control_command_t *command) {
    return syneeg_dock_coordinator_abort_segment(coordinator, command);
}

syneeg_status_t syneeg_dock_coordinator_abort_segment(
    syneeg_dock_coordinator_t *coordinator, syneeg_control_command_t *command) {
    if (coordinator == NULL || command == NULL) {
        return SYNEEG_ERROR_ARGUMENT;
    }
    if (coordinator->state != SYNEEG_DOCK_RUNNING &&
        coordinator->state != SYNEEG_DOCK_STOP_REQUIRED) {
        return SYNEEG_ERROR_STATE;
    }
    populate_command(
        coordinator, SYNEEG_ADDRESS_BROADCAST, SYNEEG_CONTROL_STOP, command);
    coordinator->state = SYNEEG_DOCK_SEGMENT_REQUIRED;
    coordinator->segment_id = 0u;
    coordinator->active_topology_generation = 0u;
    return SYNEEG_OK;
}

void syneeg_pod_node_init(syneeg_pod_node_t *node,
                          uint32_t node_id,
                          uint8_t address,
                          uint32_t authority_id) {
    if (node == NULL) {
        return;
    }
    memset(node, 0, sizeof(*node));
    node->node_id = node_id;
    node->address = address;
    node->authority_id = authority_id;
    node->state = SYNEEG_POD_UNBOUND;
}

syneeg_status_t syneeg_pod_node_apply(syneeg_pod_node_t *node,
                                      const syneeg_control_command_t *command) {
    if (node == NULL || command == NULL) {
        return SYNEEG_ERROR_ARGUMENT;
    }
    if (command->authority_id != node->authority_id) {
        return SYNEEG_ERROR_AUTHORITY;
    }
    if (command->destination_address != SYNEEG_ADDRESS_BROADCAST &&
        command->destination_address != node->address) {
        return SYNEEG_ERROR_ADDRESS;
    }
    if (!serial_is_newer(command->sequence, node->last_command_sequence)) {
        return SYNEEG_ERROR_STALE;
    }

    if (command->type == SYNEEG_CONTROL_TOPOLOGY) {
        if (node->state == SYNEEG_POD_RUNNING) {
            return SYNEEG_ERROR_STATE;
        }
        if (command->topology_generation == 0u) {
            return SYNEEG_ERROR_FORMAT;
        }
        if (command->topology_generation != node->topology_generation &&
            !serial_is_newer(command->topology_generation, node->topology_generation)) {
            return SYNEEG_ERROR_STALE;
        }
        node->topology_generation = command->topology_generation;
        node->segment_id = 0u;
        node->state = SYNEEG_POD_IDLE;
    } else if (command->type == SYNEEG_CONTROL_START) {
        if (node->state != SYNEEG_POD_IDLE) {
            return SYNEEG_ERROR_STATE;
        }
        if (command->topology_generation == 0u ||
            command->topology_generation != node->topology_generation) {
            return SYNEEG_ERROR_STALE;
        }
        if (command->segment_id == 0u) {
            return SYNEEG_ERROR_FORMAT;
        }
        if (!serial_is_newer(command->segment_id, node->last_segment_id)) {
            return SYNEEG_ERROR_STALE;
        }
        node->segment_id = command->segment_id;
        node->last_segment_id = command->segment_id;
        node->state = SYNEEG_POD_RUNNING;
    } else if (command->type == SYNEEG_CONTROL_STOP) {
        if (node->state != SYNEEG_POD_RUNNING) {
            return SYNEEG_ERROR_STATE;
        }
        if (command->topology_generation != node->topology_generation ||
            command->segment_id != node->segment_id) {
            return SYNEEG_ERROR_STALE;
        }
        node->segment_id = 0u;
        node->state = SYNEEG_POD_IDLE;
    } else {
        return SYNEEG_ERROR_ARGUMENT;
    }
    node->last_command_sequence = command->sequence;
    return SYNEEG_OK;
}
