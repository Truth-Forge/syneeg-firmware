#include "test.h"

#include "syneeg/coordinator.h"

int test_coordinator(void) {
    syneeg_coordinator_t coordinator;
    syneeg_coordinator_init(&coordinator, 20u);
    TEST_ASSERT(coordinator.state == SYNEEG_COORDINATOR_SOLO);

    TEST_ASSERT(syneeg_coordinator_observe(&coordinator, 30u, 100u, true) == SYNEEG_OK);
    TEST_ASSERT(syneeg_coordinator_observe(&coordinator, 10u, 100u, true) == SYNEEG_OK);
    TEST_ASSERT(syneeg_coordinator_elect(&coordinator, 110u, 1000u) == SYNEEG_OK);
    TEST_ASSERT(coordinator.leader_id == 10u);
    TEST_ASSERT(coordinator.state == SYNEEG_COORDINATOR_FOLLOWER);

    syneeg_coordinator_commit_epoch(&coordinator, 7u);
    TEST_ASSERT(coordinator.segment_id == 7u);
    TEST_ASSERT(!coordinator.membership_dirty);

    TEST_ASSERT(syneeg_coordinator_observe(&coordinator, 15u, 120u, true) == SYNEEG_OK);
    TEST_ASSERT(syneeg_coordinator_elect(&coordinator, 130u, 1000u) == SYNEEG_OK);
    TEST_ASSERT(coordinator.leader_id == 10u);
    TEST_ASSERT(coordinator.state == SYNEEG_COORDINATOR_SEGMENT_REQUIRED);

    syneeg_coordinator_commit_epoch(&coordinator, 8u);
    TEST_ASSERT(syneeg_coordinator_elect(&coordinator, 2000u, 1000u) == SYNEEG_OK);
    TEST_ASSERT(coordinator.leader_id == 20u);
    TEST_ASSERT(coordinator.state == SYNEEG_COORDINATOR_SEGMENT_REQUIRED);

    syneeg_coordinator_t leader;
    syneeg_coordinator_init(&leader, 10u);
    TEST_ASSERT(syneeg_coordinator_observe(&leader, 30u, 100u, true) == SYNEEG_OK);
    TEST_ASSERT(syneeg_coordinator_elect(&leader, 110u, 1000u) == SYNEEG_OK);
    syneeg_coordinator_commit_epoch(&leader, 1u);
    TEST_ASSERT(syneeg_coordinator_elect(&leader, 2000u, 1000u) == SYNEEG_OK);
    TEST_ASSERT(leader.leader_id == 10u);
    TEST_ASSERT(leader.state == SYNEEG_COORDINATOR_SEGMENT_REQUIRED);
    return 0;
}
