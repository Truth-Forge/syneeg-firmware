cmake_minimum_required(VERSION 3.20)
project(syneeg_firmware VERSION 0.1.0 LANGUAGES C)

set(CMAKE_C_STANDARD 11)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_C_EXTENSIONS OFF)

option(SYNEEG_BUILD_TESTS "Build host-side unit tests" ON)

add_library(syneeg_core STATIC
    src/ads1299.c
    src/coordinator.c
    src/protocol.c
)

target_include_directories(syneeg_core PUBLIC include)

if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
    target_compile_options(syneeg_core PRIVATE
        -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror
    )
endif()

if(SYNEEG_BUILD_TESTS)
    enable_testing()
    add_executable(syneeg_tests
        tests/test_main.c
        tests/test_ads1299.c
        tests/test_coordinator.c
        tests/test_protocol.c
    )
    target_link_libraries(syneeg_tests PRIVATE syneeg_core)
    target_include_directories(syneeg_tests PRIVATE tests)
    if(CMAKE_C_COMPILER_ID MATCHES "Clang|GNU")
        target_compile_options(syneeg_tests PRIVATE
            -Wall -Wextra -Wpedantic -Wconversion -Wshadow -Werror
        )
    endif()
    add_test(NAME syneeg_tests COMMAND syneeg_tests)
endif()
