#include "test.h"

int main(void) {
    int failures = 0;
    failures += test_ads1299();
    failures += test_protocol();
    failures += test_coordinator();
    if (failures == 0) {
        puts("all SynEEG tests passed");
    }
    return failures == 0 ? 0 : 1;
}
