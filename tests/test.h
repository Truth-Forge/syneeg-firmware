#ifndef SYNEEG_TEST_H
#define SYNEEG_TEST_H

#include <stdio.h>

#define TEST_ASSERT(condition)                                                     \
    do {                                                                           \
        if (!(condition)) {                                                         \
            fprintf(stderr, "%s:%d assertion failed: %s\n", __FILE__, __LINE__,  \
                    #condition);                                                    \
            return 1;                                                              \
        }                                                                          \
    } while (0)

int test_ads1299(void);
int test_protocol(void);
int test_coordinator(void);

#endif
