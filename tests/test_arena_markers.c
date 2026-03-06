/*********************************************************************
 * @file test_arena_markers.c
 * @brief Marker, rewind, and state-transition tests for the arena allocator.
 *********************************************************************/

#include "test_arena.h"

static bool test_get_used_transitions(void);
static bool test_marker_null_arena(void);
static bool test_invalid_rewind_marker_no_effect(void);
static bool test_demo_usage_flow(void);

static bool
test_get_used_transitions(void)
{
        static uint8_t buffer[DEMO_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        void *ptr;
        arena_marker_t mk;
        bool passed = true;

        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: arena_init failed in test_get_used_transitions\n");
                return false;
        }

        if (arena_get_used(&arena) != 0U) {
                printf("FAIL: Expected used=0 after init, got %zu\n",
                       arena_get_used(&arena));
                passed = false;
        }

        st = arena_alloc(&arena, &ptr, 16U, 1U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: alloc(16) failed\n");
                passed = false;
        }
        if (arena_get_used(&arena) != 16U) {
                printf("FAIL: Expected used=16 after alloc(16), got %zu\n",
                       arena_get_used(&arena));
                passed = false;
        }

        mk = arena_get_marker(&arena);
        st = arena_alloc(&arena, &ptr, 8U, 1U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: alloc(8) failed\n");
                passed = false;
        }
        if (arena_get_used(&arena) != 24U) {
                printf("FAIL: Expected used=24 after alloc(8), got %zu\n",
                       arena_get_used(&arena));
                passed = false;
        }

        arena_rewind(&arena, mk);
        if (arena_get_used(&arena) != 16U) {
                printf("FAIL: Expected used=16 after rewind, got %zu\n",
                       arena_get_used(&arena));
                passed = false;
        }

        arena_reset(&arena);
        if (arena_get_used(&arena) != 0U) {
                printf("FAIL: Expected used=0 after reset, got %zu\n",
                       arena_get_used(&arena));
                passed = false;
        }

        if (passed) {
                printf("PASS: test_get_used_transitions\n");
        }
        return passed;
}

static bool
test_marker_null_arena(void)
{
        static uint8_t buffer[DEMO_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        arena_marker_t mk;
        bool passed = true;

        mk = arena_get_marker(NULL);
        if (mk != ARENA_MARKER_INVALID) {
                printf("FAIL: arena_get_marker(NULL) returned %zu, "
                       "expected ARENA_MARKER_INVALID (%zu)\n",
                       mk, ARENA_MARKER_INVALID);
                passed = false;
        }

        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: arena_init failed in test_marker_null_arena\n");
                return false;
        }
        mk = arena_get_marker(&arena);
        if (mk != 0U) {
                printf("FAIL: arena_get_marker on empty arena returned %zu, "
                       "expected 0\n",
                       mk);
                passed = false;
        }
        if (mk == ARENA_MARKER_INVALID) {
                printf("FAIL: valid marker equals ARENA_MARKER_INVALID - "
                       "sentinel is not distinct from offset 0\n");
                passed = false;
        }

        if (passed) {
                printf("PASS: test_marker_null_arena\n");
        }
        return passed;
}

static bool
test_invalid_rewind_marker_no_effect(void)
{
        static uint8_t buffer[DEMO_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        void *ptr;
        const arena_marker_t invalid_marker =
            (arena_marker_t)(DEMO_BUF_SIZE + 1U);
        bool passed = true;

        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: arena_init failed in "
                       "test_invalid_rewind_marker_no_effect\n");
                return false;
        }

        st = arena_alloc(&arena, &ptr, 24U, 1U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: alloc(24) failed\n");
                return false;
        }

        arena_rewind(&arena, invalid_marker);

        if (arena_get_used(&arena) != 24U) {
                printf("FAIL: Invalid rewind changed used size to %zu\n",
                       arena_get_used(&arena));
                passed = false;
        }

        if (arena_get_high_water(&arena) != 24U) {
                printf("FAIL: Invalid rewind changed high-water mark to %zu\n",
                       arena_get_high_water(&arena));
                passed = false;
        }

        if (passed) {
                printf("PASS: test_invalid_rewind_marker_no_effect\n");
        }
        return passed;
}

static bool
test_demo_usage_flow(void)
{
        static uint8_t buffer[DEMO_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        void *p1;
        void *p2;
        void *p3;
        arena_marker_t mk;
        size_t used;
        size_t high;
        size_t cap;
        bool passed = true;

        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: arena_init failed in test_demo_usage_flow\n");
                return false;
        }

        cap = arena_get_capacity(&arena);
        if (cap != sizeof(buffer)) {
                printf("FAIL: Expected capacity=%zu, got %zu\n", sizeof(buffer),
                       cap);
                passed = false;
        }

        st = arena_alloc(&arena, &p1, 24U, 0U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: arena_alloc failed for p1\n");
                return false;
        }

        st = arena_alloc(&arena, &p2, 13U, 16U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: arena_alloc failed for p2\n");
                return false;
        }

        mk = arena_get_marker(&arena);
        if (mk == ARENA_MARKER_INVALID) {
                printf("FAIL: arena_get_marker returned invalid marker\n");
                return false;
        }

        st = arena_alloc(&arena, &p3, 40U, 0U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: arena_alloc failed for p3\n");
                return false;
        }

        used = arena_get_used(&arena);
        high = arena_get_high_water(&arena);
        if ((used != 88U) || (high != 88U)) {
                printf("FAIL: Expected used=88 and high_water=88, got %zu/%zu\n",
                       used, high);
                passed = false;
        }

        st = arena_alloc(&arena, &p1, 100U, 0U);
        if (st != ARENA_STATUS_OUT_OF_MEMORY) {
                printf("FAIL: Expected OUT_OF_MEMORY for oversized alloc, got %d\n",
                       st);
                passed = false;
        }

        arena_rewind(&arena, mk);
        if (arena_get_used(&arena) != 45U) {
                printf("FAIL: Expected used=45 after rewind, got %zu\n",
                       arena_get_used(&arena));
                passed = false;
        }

        arena_reset(&arena);
        if (arena_get_used(&arena) != 0U) {
                printf("FAIL: Expected used=0 after reset, got %zu\n",
                       arena_get_used(&arena));
                passed = false;
        }
        if (arena_get_high_water(&arena) != 88U) {
                printf("FAIL: Expected high_water=88 after reset, got %zu\n",
                       arena_get_high_water(&arena));
                passed = false;
        }

        if (passed) {
                printf("PASS: test_demo_usage_flow\n");
        }
        return passed;
}

bool
run_marker_tests(void)
{
        bool all_passed = true;

        all_passed = test_get_used_transitions() && all_passed;
        all_passed = test_marker_null_arena() && all_passed;
        all_passed = test_invalid_rewind_marker_no_effect() && all_passed;
        all_passed = test_demo_usage_flow() && all_passed;

        return all_passed;
}
