#include "test.h"

#include "syneeg/coordinator.h"

int test_coordinator(void) {
    syneeg_dock_coordinator_t dock;
    TEST_ASSERT(syneeg_dock_coordinator_init(&dock, 0xD0C0u) == SYNEEG_OK);
    TEST_ASSERT(dock.state == SYNEEG_DOCK_IDLE);
    TEST_ASSERT(dock.topology_generation == 0u);

    /* A one-Pod carrier still uses the permanent SynDock authority path. */
    TEST_ASSERT(syneeg_dock_coordinator_observe(&dock, 101u, 1u, 100u) == SYNEEG_OK);
    TEST_ASSERT(dock.state == SYNEEG_DOCK_SEGMENT_REQUIRED);
    TEST_ASSERT(dock.topology_generation == 1u);

    syneeg_pod_node_t pod1;
    syneeg_pod_node_init(&pod1, 101u, 1u, 0xD0C0u);
    syneeg_control_command_t command;
    TEST_ASSERT(syneeg_dock_coordinator_make_command(
                    &dock, SYNEEG_ADDRESS_BROADCAST, SYNEEG_CONTROL_TOPOLOGY, &command) ==
                SYNEEG_OK);
    TEST_ASSERT(syneeg_pod_node_apply(&pod1, &command) == SYNEEG_OK);
    TEST_ASSERT(pod1.topology_generation == 1u);

    TEST_ASSERT(syneeg_dock_coordinator_start_segment(&dock, 7u) == SYNEEG_OK);
    TEST_ASSERT(syneeg_dock_coordinator_make_command(
                    &dock, SYNEEG_ADDRESS_BROADCAST, SYNEEG_CONTROL_START, &command) ==
                SYNEEG_OK);
    const syneeg_control_command_t old_start = command;
    TEST_ASSERT(syneeg_pod_node_apply(&pod1, &command) == SYNEEG_OK);
    TEST_ASSERT(pod1.state == SYNEEG_POD_RUNNING);
    TEST_ASSERT(pod1.segment_id == 7u);
    TEST_ASSERT(syneeg_dock_coordinator_make_command(
                    &dock, SYNEEG_ADDRESS_BROADCAST, SYNEEG_CONTROL_STOP, &command) ==
                SYNEEG_ERROR_STATE);

    /* A second addressed Pod changes topology and forces a new segment. */
    TEST_ASSERT(syneeg_dock_coordinator_observe(&dock, 202u, 2u, 120u) == SYNEEG_OK);
    TEST_ASSERT(dock.state == SYNEEG_DOCK_STOP_REQUIRED);
    TEST_ASSERT(dock.topology_generation == 2u);
    syneeg_pod_node_t pod2;
    syneeg_pod_node_init(&pod2, 202u, 2u, 0xD0C0u);

    /* The old segment is explicitly stopped using its active topology. */
    TEST_ASSERT(syneeg_dock_coordinator_make_command(
                    &dock, SYNEEG_ADDRESS_BROADCAST, SYNEEG_CONTROL_TOPOLOGY, &command) ==
                SYNEEG_ERROR_STATE);
    TEST_ASSERT(syneeg_dock_coordinator_abort_segment(&dock, &command) == SYNEEG_OK);
    TEST_ASSERT(command.type == SYNEEG_CONTROL_STOP);
    TEST_ASSERT(command.topology_generation == 1u);
    TEST_ASSERT(syneeg_pod_node_apply(&pod1, &command) == SYNEEG_OK);
    TEST_ASSERT(pod1.state == SYNEEG_POD_IDLE);
    TEST_ASSERT(dock.state == SYNEEG_DOCK_SEGMENT_REQUIRED);

    TEST_ASSERT(syneeg_dock_coordinator_make_command(
                    &dock, SYNEEG_ADDRESS_BROADCAST, SYNEEG_CONTROL_TOPOLOGY, &command) ==
                SYNEEG_OK);
    TEST_ASSERT(syneeg_pod_node_apply(&pod1, &command) == SYNEEG_OK);
    TEST_ASSERT(syneeg_pod_node_apply(&pod2, &command) == SYNEEG_OK);
    TEST_ASSERT(pod1.state == SYNEEG_POD_IDLE);
    TEST_ASSERT(syneeg_pod_node_apply(&pod1, &old_start) == SYNEEG_ERROR_STALE);

    TEST_ASSERT(syneeg_dock_coordinator_start_segment(&dock, 7u) == SYNEEG_ERROR_STALE);
    TEST_ASSERT(syneeg_dock_coordinator_start_segment(&dock, 6u) == SYNEEG_ERROR_STALE);
    TEST_ASSERT(syneeg_dock_coordinator_start_segment(&dock, 8u) == SYNEEG_OK);
    TEST_ASSERT(syneeg_dock_coordinator_make_command(
                    &dock, SYNEEG_ADDRESS_BROADCAST, SYNEEG_CONTROL_START, &command) ==
                SYNEEG_OK);
    TEST_ASSERT(syneeg_pod_node_apply(&pod1, &command) == SYNEEG_OK);
    TEST_ASSERT(syneeg_pod_node_apply(&pod2, &command) == SYNEEG_OK);

    syneeg_control_command_t impostor = command;
    impostor.authority_id = 0xBADu;
    ++impostor.sequence;
    TEST_ASSERT(syneeg_pod_node_apply(&pod1, &impostor) == SYNEEG_ERROR_AUTHORITY);
    impostor = command;
    impostor.destination_address = 2u;
    ++impostor.sequence;
    TEST_ASSERT(syneeg_pod_node_apply(&pod1, &impostor) == SYNEEG_ERROR_ADDRESS);

    /* A running fault also emits STOP before the Dock enters segment-required. */
    TEST_ASSERT(syneeg_dock_coordinator_abort_segment(&dock, &command) == SYNEEG_OK);
    TEST_ASSERT(command.type == SYNEEG_CONTROL_STOP);
    TEST_ASSERT(syneeg_pod_node_apply(&pod1, &command) == SYNEEG_OK);
    TEST_ASSERT(syneeg_pod_node_apply(&pod2, &command) == SYNEEG_OK);
    TEST_ASSERT(pod1.state == SYNEEG_POD_IDLE);
    TEST_ASSERT(dock.state == SYNEEG_DOCK_SEGMENT_REQUIRED);

    TEST_ASSERT(syneeg_dock_coordinator_start_segment(&dock, 8u) == SYNEEG_ERROR_STALE);
    TEST_ASSERT(syneeg_dock_coordinator_start_segment(&dock, 7u) == SYNEEG_ERROR_STALE);
    TEST_ASSERT(syneeg_dock_coordinator_start_segment(&dock, 9u) == SYNEEG_OK);
    TEST_ASSERT(syneeg_dock_coordinator_make_command(
                    &dock, SYNEEG_ADDRESS_BROADCAST, SYNEEG_CONTROL_START, &command) ==
                SYNEEG_OK);
    TEST_ASSERT(syneeg_pod_node_apply(&pod1, &command) == SYNEEG_OK);
    TEST_ASSERT(syneeg_pod_node_apply(&pod2, &command) == SYNEEG_OK);
    TEST_ASSERT(syneeg_dock_coordinator_stop_segment(&dock, &command) == SYNEEG_OK);
    TEST_ASSERT(syneeg_pod_node_apply(&pod1, &command) == SYNEEG_OK);
    TEST_ASSERT(syneeg_pod_node_apply(&pod2, &command) == SYNEEG_OK);

    /* Node loss and explicit faults are both segment boundaries, never elections. */
    TEST_ASSERT(syneeg_dock_coordinator_observe(&dock, 101u, 1u, 1500u) == SYNEEG_OK);
    TEST_ASSERT(syneeg_dock_coordinator_expire(&dock, 2000u, 1000u) == SYNEEG_OK);
    TEST_ASSERT(dock.active_node_count == 1u);
    TEST_ASSERT(dock.topology_generation == 3u);
    TEST_ASSERT(dock.state == SYNEEG_DOCK_SEGMENT_REQUIRED);
    TEST_ASSERT(syneeg_dock_coordinator_make_command(
                    &dock, SYNEEG_ADDRESS_BROADCAST, SYNEEG_CONTROL_TOPOLOGY, &command) ==
                SYNEEG_OK);
    TEST_ASSERT(syneeg_pod_node_apply(&pod1, &command) == SYNEEG_OK);
    TEST_ASSERT(syneeg_dock_coordinator_start_segment(&dock, 9u) == SYNEEG_ERROR_STALE);
    TEST_ASSERT(syneeg_dock_coordinator_start_segment(&dock, 8u) == SYNEEG_ERROR_STALE);
    TEST_ASSERT(syneeg_dock_coordinator_start_segment(&dock, 10u) == SYNEEG_OK);

    /* Reactivation validates the full address table before mutating a known node. */
    syneeg_dock_coordinator_t addresses;
    TEST_ASSERT(syneeg_dock_coordinator_init(&addresses, 1u) == SYNEEG_OK);
    TEST_ASSERT(syneeg_dock_coordinator_observe(&addresses, 11u, 1u, 0u) == SYNEEG_OK);
    TEST_ASSERT(syneeg_dock_coordinator_observe(&addresses, 22u, 2u, 0u) == SYNEEG_OK);
    TEST_ASSERT(syneeg_dock_coordinator_observe(&addresses, 22u, 2u, 100u) == SYNEEG_OK);
    TEST_ASSERT(syneeg_dock_coordinator_expire(&addresses, 150u, 75u) == SYNEEG_OK);
    TEST_ASSERT(addresses.active_node_count == 1u);
    const uint32_t topology_before_conflict = addresses.topology_generation;
    TEST_ASSERT(syneeg_dock_coordinator_observe(&addresses, 11u, 2u, 151u) ==
                SYNEEG_ERROR_ADDRESS);
    TEST_ASSERT(!addresses.nodes[0].active);
    TEST_ASSERT(addresses.active_node_count == 1u);
    TEST_ASSERT(addresses.topology_generation == topology_before_conflict);

    /* Inactive slots are reusable; capacity means eight active identities. */
    syneeg_dock_coordinator_t replacements;
    TEST_ASSERT(syneeg_dock_coordinator_init(&replacements, 2u) == SYNEEG_OK);
    for (size_t i = 0u; i < SYNEEG_MAX_ACTIVE_NODES; ++i) {
        TEST_ASSERT(syneeg_dock_coordinator_observe(
                        &replacements, 1000u + (uint32_t)i, (uint8_t)(i + 1u), 0u) ==
                    SYNEEG_OK);
    }
    TEST_ASSERT(syneeg_dock_coordinator_expire(&replacements, 100u, 50u) == SYNEEG_OK);
    TEST_ASSERT(replacements.active_node_count == 0u);
    TEST_ASSERT(replacements.node_count == SYNEEG_MAX_ACTIVE_NODES);
    TEST_ASSERT(syneeg_dock_coordinator_observe(&replacements, 9999u, 1u, 101u) ==
                SYNEEG_OK);
    TEST_ASSERT(replacements.active_node_count == 1u);
    TEST_ASSERT(replacements.node_count == SYNEEG_MAX_ACTIVE_NODES);

    /* RFC1982-style serial arithmetic skips zero and accepts max-to-one rollover. */
    syneeg_dock_coordinator_t rollover;
    TEST_ASSERT(syneeg_dock_coordinator_init(&rollover, 3u) == SYNEEG_OK);
    rollover.topology_generation = UINT32_MAX;
    TEST_ASSERT(syneeg_dock_coordinator_observe(&rollover, 33u, 1u, 0u) == SYNEEG_OK);
    TEST_ASSERT(rollover.topology_generation == 1u);
    rollover.next_command_sequence = UINT32_MAX;
    syneeg_control_command_t rollover_command;
    TEST_ASSERT(syneeg_dock_coordinator_make_command(
                    &rollover,
                    SYNEEG_ADDRESS_BROADCAST,
                    SYNEEG_CONTROL_TOPOLOGY,
                    &rollover_command) == SYNEEG_OK);
    TEST_ASSERT(rollover_command.sequence == 1u);
    syneeg_pod_node_t rollover_pod;
    syneeg_pod_node_init(&rollover_pod, 33u, 1u, 3u);
    rollover_pod.state = SYNEEG_POD_IDLE;
    rollover_pod.topology_generation = UINT32_MAX;
    rollover_pod.last_command_sequence = UINT32_MAX;
    rollover_pod.last_segment_id = UINT32_MAX;
    TEST_ASSERT(syneeg_pod_node_apply(&rollover_pod, &rollover_command) == SYNEEG_OK);
    TEST_ASSERT(rollover_pod.topology_generation == 1u);
    rollover.last_segment_id = UINT32_MAX;
    TEST_ASSERT(syneeg_dock_coordinator_start_segment(&rollover, 1u) == SYNEEG_OK);
    TEST_ASSERT(syneeg_dock_coordinator_make_command(
                    &rollover,
                    SYNEEG_ADDRESS_BROADCAST,
                    SYNEEG_CONTROL_START,
                    &rollover_command) == SYNEEG_OK);
    TEST_ASSERT(syneeg_pod_node_apply(&rollover_pod, &rollover_command) == SYNEEG_OK);
    TEST_ASSERT(rollover_pod.segment_id == 1u);
    syneeg_control_command_t old_serial = rollover_command;
    old_serial.sequence = UINT32_MAX;
    TEST_ASSERT(syneeg_pod_node_apply(&rollover_pod, &old_serial) == SYNEEG_ERROR_STALE);
    TEST_ASSERT(syneeg_dock_coordinator_abort_segment(&rollover, &rollover_command) ==
                SYNEEG_OK);
    TEST_ASSERT(syneeg_pod_node_apply(&rollover_pod, &rollover_command) == SYNEEG_OK);
    old_serial = rollover_command;
    old_serial.type = SYNEEG_CONTROL_TOPOLOGY;
    old_serial.segment_id = 0u;
    old_serial.topology_generation = UINT32_MAX;
    old_serial.sequence = 4u;
    TEST_ASSERT(syneeg_pod_node_apply(&rollover_pod, &old_serial) == SYNEEG_ERROR_STALE);
    TEST_ASSERT(syneeg_dock_coordinator_start_segment(&rollover, UINT32_MAX) ==
                SYNEEG_ERROR_STALE);

    /* Pod state transitions reject out-of-order and cross-segment commands. */
    syneeg_pod_node_t guarded;
    syneeg_pod_node_init(&guarded, 44u, 4u, 4u);
    syneeg_control_command_t guarded_command = {
        .authority_id = 4u,
        .segment_id = 7u,
        .topology_generation = 1u,
        .sequence = 1u,
        .destination_address = 4u,
        .type = SYNEEG_CONTROL_START
    };
    TEST_ASSERT(syneeg_pod_node_apply(&guarded, &guarded_command) == SYNEEG_ERROR_STATE);
    guarded_command.type = SYNEEG_CONTROL_TOPOLOGY;
    guarded_command.segment_id = 0u;
    TEST_ASSERT(syneeg_pod_node_apply(&guarded, &guarded_command) == SYNEEG_OK);
    guarded_command.type = SYNEEG_CONTROL_START;
    guarded_command.sequence = 2u;
    TEST_ASSERT(syneeg_pod_node_apply(&guarded, &guarded_command) == SYNEEG_ERROR_FORMAT);
    guarded_command.segment_id = 7u;
    guarded_command.topology_generation = 2u;
    TEST_ASSERT(syneeg_pod_node_apply(&guarded, &guarded_command) == SYNEEG_ERROR_STALE);
    guarded_command.topology_generation = 1u;
    TEST_ASSERT(syneeg_pod_node_apply(&guarded, &guarded_command) == SYNEEG_OK);
    guarded_command.type = SYNEEG_CONTROL_TOPOLOGY;
    guarded_command.topology_generation = 2u;
    guarded_command.sequence = 3u;
    TEST_ASSERT(syneeg_pod_node_apply(&guarded, &guarded_command) == SYNEEG_ERROR_STATE);
    guarded_command.type = SYNEEG_CONTROL_START;
    guarded_command.topology_generation = 1u;
    guarded_command.segment_id = 8u;
    TEST_ASSERT(syneeg_pod_node_apply(&guarded, &guarded_command) == SYNEEG_ERROR_STATE);
    guarded_command.type = SYNEEG_CONTROL_STOP;
    TEST_ASSERT(syneeg_pod_node_apply(&guarded, &guarded_command) == SYNEEG_ERROR_STALE);
    guarded_command.segment_id = 7u;
    TEST_ASSERT(syneeg_pod_node_apply(&guarded, &guarded_command) == SYNEEG_OK);
    TEST_ASSERT(guarded.state == SYNEEG_POD_IDLE);
    TEST_ASSERT(syneeg_pod_node_apply(&guarded, &guarded_command) == SYNEEG_ERROR_STALE);
    guarded_command.sequence = 4u;
    TEST_ASSERT(syneeg_pod_node_apply(&guarded, &guarded_command) == SYNEEG_ERROR_STATE);
    return 0;
}
