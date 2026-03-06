/*********************************************************************
 * @file test_arena_poison.c
 * @brief Debug-poison regression tests for the arena allocator.
 *
 * This suite is built with ARENA_CFG_DEBUG_POISON enabled so the tests can
 * verify the documented poisoning policy across init, rewind, and reset.
 *
 *********************************************************************/

#include "arena.h"
#include <stdbool.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#define POISON_BUF_SIZE (64U)

static bool test_init_poisons_entire_buffer(void);
static bool test_rewind_poisons_free_tail(void);
static bool test_reset_poisons_entire_buffer(void);

static bool
buffer_has_pattern(const uint8_t *const buffer, const size_t size,
                   const uint8_t pattern)
{
        size_t i;

        for (i = 0U; i < size; ++i) {
                if (buffer[i] != pattern) {
                        return false;
                }
        }

        return true;
}

static bool
test_init_poisons_entire_buffer(void)
{
        static uint8_t buffer[POISON_BUF_SIZE];
        arena_t arena;
        arena_status_t st;

        memset(buffer, 0x11, sizeof(buffer));

        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: arena_init failed in "
                       "test_init_poisons_entire_buffer\n");
                return false;
        }

        if (!buffer_has_pattern(buffer, sizeof(buffer),
                                ARENA_CFG_POISON_PATTERN)) {
                printf("FAIL: arena_init did not poison entire buffer\n");
                return false;
        }

        printf("PASS: test_init_poisons_entire_buffer\n");
        return true;
}

static bool
test_rewind_poisons_free_tail(void)
{
        static uint8_t buffer[POISON_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        void *ptr1;
        void *ptr2;
        arena_marker_t marker;
        bool passed = true;

        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: arena_init failed in "
                       "test_rewind_poisons_free_tail\n");
                return false;
        }

        st = arena_alloc(&arena, &ptr1, 16U, 1U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: alloc(ptr1) failed\n");
                return false;
        }
        memset(ptr1, 0x11, 16U);

        marker = arena_get_marker(&arena);

        st = arena_alloc(&arena, &ptr2, 16U, 1U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: alloc(ptr2) failed\n");
                return false;
        }
        memset(ptr2, 0x22, 16U);

        memset(&buffer[40], 0x33, 8U);

        arena_rewind(&arena, marker);

        if (buffer[0] != 0x11U) {
                printf("FAIL: rewind modified live prefix\n");
                passed = false;
        }

        if (!buffer_has_pattern(&buffer[16], sizeof(buffer) - 16U,
                                ARENA_CFG_POISON_PATTERN)) {
                printf("FAIL: rewind did not poison full free tail\n");
                passed = false;
        }

        if (passed) {
                printf("PASS: test_rewind_poisons_free_tail\n");
        }
        return passed;
}

static bool
test_reset_poisons_entire_buffer(void)
{
        static uint8_t buffer[POISON_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        void *ptr;

        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: arena_init failed in "
                       "test_reset_poisons_entire_buffer\n");
                return false;
        }

        st = arena_alloc(&arena, &ptr, 24U, 1U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: alloc(ptr) failed\n");
                return false;
        }
        memset(ptr, 0x44, 24U);

        arena_reset(&arena);

        if (!buffer_has_pattern(buffer, sizeof(buffer),
                                ARENA_CFG_POISON_PATTERN)) {
                printf("FAIL: arena_reset did not re-poison entire buffer\n");
                return false;
        }

        printf("PASS: test_reset_poisons_entire_buffer\n");
        return true;
}

int
main(void)
{
        bool all_passed = true;

        all_passed = test_init_poisons_entire_buffer() && all_passed;
        all_passed = test_rewind_poisons_free_tail() && all_passed;
        all_passed = test_reset_poisons_entire_buffer() && all_passed;

        if (!all_passed) {
                printf("Some poison tests failed!\n");
                return EXIT_FAILURE;
        }

        return EXIT_SUCCESS;
}
