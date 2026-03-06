/*********************************************************************
 * @file test_arena.c
 * @brief Test runner for the arena allocator unit suite.
 *********************************************************************/

#include "test_arena.h"

int
main(void)
{
        bool all_passed = true;

        all_passed = run_api_tests() && all_passed;
        all_passed = run_allocation_tests() && all_passed;
        all_passed = run_marker_tests() && all_passed;

        if (!all_passed) {
                printf("Some tests failed!\n");
                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}
