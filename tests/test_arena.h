/*********************************************************************
 * @file test_arena.h
 * @brief Shared declarations for the arena unit test suite.
 *********************************************************************/

#ifndef TEST_ARENA_H_
#define TEST_ARENA_H_

#include "arena.h"
#include <stdbool.h>

/* MISRA C:2012 Rule 21.6: stdio permitted only in test/debug builds */
#if defined(TEST_BUILD)
#include <stdio.h>
#include <stdlib.h> /* EXIT_FAILURE, EXIT_SUCCESS */
#endif

#define DEMO_BUF_SIZE (128U)

extern bool run_api_tests(void);
extern bool run_allocation_tests(void);
extern bool run_marker_tests(void);

#endif /* TEST_ARENA_H_ */
