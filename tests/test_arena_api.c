/*********************************************************************
 * @file test_arena_api.c
 * @brief API validation tests for the arena allocator.
 *********************************************************************/

#include "test_arena.h"

static bool test_invalid_alignment(void);
static bool test_zero_size_allocation(void);
static bool test_null_pointers(void);
static bool test_invalid_argument(void);
static bool test_query_null_helpers(void);
static bool test_failed_alloc_clears_result(void);

static bool
test_invalid_alignment(void)
{
        static uint8_t buffer[DEMO_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        void *ptr;
        bool passed = true;

        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: arena_init failed in test_invalid_alignment\n");
                return false;
        }

        st = arena_alloc(&arena, &ptr, 16U, 3U);
        if (st != ARENA_STATUS_INVALID_ALIGNMENT) {
                printf("FAIL: Expected ARENA_STATUS_INVALID_ALIGNMENT for "
                       "alignment=3, got %d\n",
                       st);
                passed = false;
        }

        st = arena_alloc(&arena, &ptr, 16U, 0U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: Default alignment (0) should work\n");
                passed = false;
        }

        st = arena_alloc(&arena, &ptr, 16U, 1U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: Alignment=1 should work\n");
                passed = false;
        }

        if (passed) {
                printf("PASS: test_invalid_alignment\n");
        }
        return passed;
}

static bool
test_zero_size_allocation(void)
{
        static uint8_t buffer[DEMO_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        void *ptr;
        bool passed = true;

        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf(
                    "FAIL: arena_init failed in test_zero_size_allocation\n");
                return false;
        }

        st = arena_alloc(&arena, &ptr, 0U, 0U);
        if (st != ARENA_STATUS_INVALID_ARGUMENT) {
                printf("FAIL: Expected ARENA_STATUS_INVALID_ARGUMENT for "
                       "size=0, got %d\n",
                       st);
                passed = false;
        }

        if (passed) {
                printf("PASS: test_zero_size_allocation\n");
        }
        return passed;
}

static bool
test_null_pointers(void)
{
        static uint8_t buffer[DEMO_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        void *ptr;
        bool passed = true;

        st = arena_alloc(NULL, &ptr, 16U, 0U);
        if (st != ARENA_STATUS_NULL_POINTER) {
                printf("FAIL: Expected ARENA_STATUS_NULL_POINTER for NULL "
                       "arena, got %d\n",
                       st);
                passed = false;
        }

        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: arena_init failed in test_null_pointers\n");
                return false;
        }
        st = arena_alloc(&arena, NULL, 16U, 0U);
        if (st != ARENA_STATUS_NULL_POINTER) {
                printf("FAIL: Expected ARENA_STATUS_NULL_POINTER for NULL "
                       "result, got %d\n",
                       st);
                passed = false;
        }

        st = arena_init(&arena, NULL, sizeof(buffer));
        if (st != ARENA_STATUS_NULL_POINTER) {
                printf("FAIL: Expected ARENA_STATUS_NULL_POINTER for NULL "
                       "buffer, got %d\n",
                       st);
                passed = false;
        }

        if (passed) {
                printf("PASS: test_null_pointers\n");
        }
        return passed;
}

static bool
test_invalid_argument(void)
{
        static uint8_t buffer[DEMO_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        void *ptr;
        bool passed = true;

        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: arena_init failed in test_invalid_argument\n");
                return false;
        }

        st = arena_init(&arena, buffer, 0U);
        if (st != ARENA_STATUS_INVALID_ARGUMENT) {
                printf("FAIL: Expected INVALID_ARGUMENT for arena_init size=0, "
                       "got %d\n",
                       st);
                passed = false;
        }

        (void)arena_init(&arena, buffer, sizeof(buffer));

        st = arena_alloc(&arena, &ptr, 0U, 0U);
        if (st != ARENA_STATUS_INVALID_ARGUMENT) {
                printf("FAIL: Expected INVALID_ARGUMENT for alloc size=0 "
                       "align=0, got %d\n",
                       st);
                passed = false;
        }

        st = arena_alloc(&arena, &ptr, 0U, 3U);
        if (st != ARENA_STATUS_INVALID_ALIGNMENT) {
                printf("FAIL: Expected INVALID_ALIGNMENT for align=3 size=0, "
                       "got %d\n",
                       st);
                passed = false;
        }

        st = arena_alloc(&arena, &ptr, 0U, 8U);
        if (st != ARENA_STATUS_INVALID_ARGUMENT) {
                printf("FAIL: Expected INVALID_ARGUMENT for align=8 size=0, "
                       "got %d\n",
                       st);
                passed = false;
        }

        if (passed) {
                printf("PASS: test_invalid_argument\n");
        }
        return passed;
}

static bool
test_query_null_helpers(void)
{
        bool passed = true;

        if (arena_get_used(NULL) != 0U) {
                printf("FAIL: arena_get_used(NULL) did not return 0\n");
                passed = false;
        }

        if (arena_get_high_water(NULL) != 0U) {
                printf("FAIL: arena_get_high_water(NULL) did not return 0\n");
                passed = false;
        }

        if (arena_get_capacity(NULL) != 0U) {
                printf("FAIL: arena_get_capacity(NULL) did not return 0\n");
                passed = false;
        }

        if (passed) {
                printf("PASS: test_query_null_helpers\n");
        }
        return passed;
}

static bool
test_failed_alloc_clears_result(void)
{
        static uint8_t buffer[DEMO_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        void *ptr = buffer;
        bool passed = true;

        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: arena_init failed in "
                       "test_failed_alloc_clears_result\n");
                return false;
        }

        st = arena_alloc(&arena, &ptr, 0U, 1U);
        if ((st != ARENA_STATUS_INVALID_ARGUMENT) || (ptr != NULL)) {
                printf("FAIL: invalid-argument alloc did not clear result\n");
                passed = false;
        }

        ptr = buffer;
        st = arena_alloc(&arena, &ptr, 1U, 3U);
        if ((st != ARENA_STATUS_INVALID_ALIGNMENT) || (ptr != NULL)) {
                printf("FAIL: invalid-alignment alloc did not clear result\n");
                passed = false;
        }

        ptr = buffer;
        st = arena_alloc(&arena, &ptr, sizeof(buffer), 1U);
        if (st != ARENA_STATUS_OK) {
                printf(
                    "FAIL: exact-capacity alloc failed in result-clear test\n");
                return false;
        }

        st = arena_alloc(&arena, &ptr, 1U, 1U);
        if ((st != ARENA_STATUS_OUT_OF_MEMORY) || (ptr != NULL)) {
                printf("FAIL: out-of-memory alloc did not clear result\n");
                passed = false;
        }

        ptr = buffer;
        st = arena_alloc(NULL, &ptr, 1U, 1U);
        if ((st != ARENA_STATUS_NULL_POINTER) || (ptr != NULL)) {
                printf("FAIL: NULL arena alloc did not clear result\n");
                passed = false;
        }

        if (passed) {
                printf("PASS: test_failed_alloc_clears_result\n");
        }
        return passed;
}

bool
run_api_tests(void)
{
        bool all_passed = true;

        all_passed = test_invalid_alignment() && all_passed;
        all_passed = test_zero_size_allocation() && all_passed;
        all_passed = test_null_pointers() && all_passed;
        all_passed = test_invalid_argument() && all_passed;
        all_passed = test_query_null_helpers() && all_passed;
        all_passed = test_failed_alloc_clears_result() && all_passed;

        return all_passed;
}
