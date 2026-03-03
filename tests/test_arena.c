/*********************************************************************
 * @file test_arena.c
 * @brief Minimal demonstration of the arena allocator.
 *
 * Compile (e.g. with GCC):
 *   gcc -Wall -Wextra -std=c11 -DTEST_BUILD arena.c arena_demo.c -o arena_demo
 *
 * Run:
 *   ./arena_demo
 *
 * The demo checks:
 *   - Simple allocation & alignment
 *   - Over-allocation detection
 *   - Marker / rewind usage
 *   - High-water mark tracking
 *
 *********************************************************************/

#include "arena.h"
#include <stdbool.h>
/* MISRA C:2012 Rule 21.6: stdio permitted only in test/debug builds */
#if defined(TEST_BUILD)
#include <stdio.h>
#include <stdlib.h> /* EXIT_FAILURE, EXIT_SUCCESS */
#endif

#define DEMO_BUF_SIZE (128U)

static bool test_invalid_alignment(void);
static bool test_zero_size_allocation(void);
static bool test_null_pointers(void);
static bool test_alignment_boundaries(void);
static bool test_invalid_argument(void);
static bool test_alloc_exact_capacity(void);
static bool test_get_used_transitions(void);
static bool test_marker_null_arena(void);
static bool test_alignment_mask_correctness(void);

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

        /* Test invalid alignment (not a power of two) */
        st = arena_alloc(&arena, &ptr, 16U, 3U); /* 3 is not a power of two */
        if (st != ARENA_STATUS_INVALID_ALIGNMENT) {
                printf("FAIL: Expected ARENA_STATUS_INVALID_ALIGNMENT for "
                       "alignment=3, got %d\n",
                       st);
                passed = false;
        }

        /* Test alignment = 0 (should use default) */
        st = arena_alloc(&arena, &ptr, 16U, 0U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: Default alignment (0) should work\n");
                passed = false;
        }

        /* Test alignment = 1 (edge case, should work) */
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

        /* Test zero size allocation */
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

        /* Test NULL arena pointer */
        st = arena_alloc(NULL, &ptr, 16U, 0U);
        if (st != ARENA_STATUS_NULL_POINTER) {
                printf("FAIL: Expected ARENA_STATUS_NULL_POINTER for NULL "
                       "arena, got %d\n",
                       st);
                passed = false;
        }

        /* Test NULL result pointer */
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

        /* Test NULL buffer in init */
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
test_alignment_boundaries(void)
{
        static uint8_t buffer[DEMO_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        void *ptr1, *ptr2, *ptr3, *ptr4;
        bool passed = true;

        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf(
                    "FAIL: arena_init failed in test_alignment_boundaries\n");
                return false;
        }

        /* Test various alignment boundaries */
        st = arena_alloc(&arena, &ptr1, 8U, 1U); /* align 1 */
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: Alignment=1 failed\n");
                passed = false;
        }

        st = arena_alloc(&arena, &ptr2, 8U, 2U); /* align 2 */
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: Alignment=2 failed\n");
                passed = false;
        }

        st = arena_alloc(&arena, &ptr3, 8U, 4U); /* align 4 */
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: Alignment=4 failed\n");
                passed = false;
        }

        st = arena_alloc(&arena, &ptr4, 8U, 8U); /* align 8 */
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: Alignment=8 failed\n");
                passed = false;
        }

        /* Verify alignments: mask = (alignment - 1), address & mask must be 0
         */
        if (((uintptr_t)ptr1 & 0U)
            != 0U) { /* align=1: every address qualifies */
                printf("FAIL: ptr1 not aligned to 1\n");
                passed = false;
        }
        if (((uintptr_t)ptr2 & 1U) != 0U) { /* align=2: bit 0 must be clear */
                printf("FAIL: ptr2 not aligned to 2\n");
                passed = false;
        }
        if (((uintptr_t)ptr3 & 3U)
            != 0U) { /* align=4: bits 1:0 must be clear */
                printf("FAIL: ptr3 not aligned to 4\n");
                passed = false;
        }
        if (((uintptr_t)ptr4 & 7U)
            != 0U) { /* align=8: bits 2:0 must be clear */
                printf("FAIL: ptr4 not aligned to 8\n");
                passed = false;
        }

        if (passed) {
                printf("PASS: test_alignment_boundaries\n");
        }
        return passed;
}

/* ------------------------------------------------------------------ */
/*   REGRESSION TESTS                                                   */
/* ------------------------------------------------------------------ */

/**
 * @brief Verify ARENA_STATUS_INVALID_ARGUMENT is returned for zero-size
 *        arguments and that guard ordering is correct.
 */
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

        /* arena_init with size == 0 */
        st = arena_init(&arena, buffer, 0U);
        if (st != ARENA_STATUS_INVALID_ARGUMENT) {
                printf("FAIL: Expected INVALID_ARGUMENT for arena_init size=0, "
                       "got %d\n",
                       st);
                passed = false;
        }

        /* Re-init for subsequent alloc tests */
        (void)arena_init(&arena, buffer, sizeof(buffer));

        /* alignment=0 resolves to default (valid), size=0 → INVALID_ARGUMENT */
        st = arena_alloc(&arena, &ptr, 0U, 0U);
        if (st != ARENA_STATUS_INVALID_ARGUMENT) {
                printf("FAIL: Expected INVALID_ARGUMENT for alloc size=0 "
                       "align=0, got %d\n",
                       st);
                passed = false;
        }

        /* alignment=3 (invalid) checked before size=0: must return
         * INVALID_ALIGNMENT */
        st = arena_alloc(&arena, &ptr, 0U, 3U);
        if (st != ARENA_STATUS_INVALID_ALIGNMENT) {
                printf("FAIL: Expected INVALID_ALIGNMENT for align=3 size=0, "
                       "got %d\n",
                       st);
                passed = false;
        }

        /* alignment=8 (valid), size=0 → INVALID_ARGUMENT */
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

/**
 * @brief Verify that allocating exactly the arena capacity succeeds and
 *        one further byte is rejected.
 */
static bool
test_alloc_exact_capacity(void)
{
        static uint8_t buffer[DEMO_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        void *ptr;
        bool passed = true;

        /* Use alignment=1 so there is zero padding */
        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf(
                    "FAIL: arena_init failed in test_alloc_exact_capacity\n");
                return false;
        }

        /* Allocate exactly the full capacity in one shot */
        st = arena_alloc(&arena, &ptr, sizeof(buffer), 1U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: Exact-capacity alloc returned %d\n", st);
                passed = false;
        }

        /* One more byte must fail */
        st = arena_alloc(&arena, &ptr, 1U, 1U);
        if (st != ARENA_STATUS_OUT_OF_MEMORY) {
                printf(
                    "FAIL: Expected OUT_OF_MEMORY after full alloc, got %d\n",
                    st);
                passed = false;
        }

        /* Fill via repeated 1-byte allocations and verify we reach the end */
        (void)arena_init(&arena, buffer, sizeof(buffer));
        {
                size_t i;
                for (i = 0U; i < sizeof(buffer); ++i) {
                        st = arena_alloc(&arena, &ptr, 1U, 1U);
                        if (st != ARENA_STATUS_OK) {
                                printf("FAIL: 1-byte alloc %zu failed\n", i);
                                passed = false;
                                break;
                        }
                }
        }
        if (arena_get_used(&arena) != sizeof(buffer)) {
                printf("FAIL: Expected used=%zu after filling, got %zu\n",
                       sizeof(buffer), arena_get_used(&arena));
                passed = false;
        }
        st = arena_alloc(&arena, &ptr, 1U, 1U);
        if (st != ARENA_STATUS_OUT_OF_MEMORY) {
                printf("FAIL: Expected OUT_OF_MEMORY at end of filled arena, "
                       "got %d\n",
                       st);
                passed = false;
        }

        if (passed) {
                printf("PASS: test_alloc_exact_capacity\n");
        }
        return passed;
}

/**
 * @brief Verify arena_get_used returns the correct value through
 *        alloc → rewind → reset transitions.
 */
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
                printf(
                    "FAIL: arena_init failed in test_get_used_transitions\n");
                return false;
        }

        /* Initially used = 0 */
        if (arena_get_used(&arena) != 0U) {
                printf("FAIL: Expected used=0 after init, got %zu\n",
                       arena_get_used(&arena));
                passed = false;
        }

        /* After 16-byte alloc (align=1) used = 16 */
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

        /* Save marker, allocate another 8 bytes */
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

        /* Rewind to marker: used goes back to 16 */
        arena_rewind(&arena, mk);
        if (arena_get_used(&arena) != 16U) {
                printf("FAIL: Expected used=16 after rewind, got %zu\n",
                       arena_get_used(&arena));
                passed = false;
        }

        /* Reset: used goes to 0 */
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

/**
 * @brief Regression test: arena_get_marker must return ARENA_MARKER_INVALID
 *        when passed a NULL arena pointer, not the ambiguous value 0.
 *
 * Prior to the fix, NULL would silently return 0 which is a valid marker
 * value, making it impossible for a caller to detect the error.
 */
static bool
test_marker_null_arena(void)
{
        static uint8_t buffer[DEMO_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        arena_marker_t mk;
        bool passed = true;

        /* NULL arena must return the sentinel, not 0 */
        mk = arena_get_marker(NULL);
        if (mk != ARENA_MARKER_INVALID) {
                printf("FAIL: arena_get_marker(NULL) returned %zu, "
                       "expected ARENA_MARKER_INVALID (%zu)\n",
                       mk, ARENA_MARKER_INVALID);
                passed = false;
        }

        /* A freshly initialised arena at offset 0 must return 0, not the
         * sentinel, confirming the two values are distinct. */
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
                printf("FAIL: valid marker equals ARENA_MARKER_INVALID — "
                       "sentinel is not distinct from offset 0\n");
                passed = false;
        }

        if (passed) {
                printf("PASS: test_marker_null_arena\n");
        }
        return passed;
}

/**
 * @brief Regression test: alignment mask checks must use the correct mask
 *        for each alignment boundary.
 *
 * Previously ptr1 (align=1) and ptr2 (align=2) used the same mask (1U),
 * meaning the ptr2 check added no additional coverage.
 */
static bool
test_alignment_mask_correctness(void)
{
        static uint8_t buffer[DEMO_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        void *ptr;
        bool passed = true;
        uintptr_t addr;

        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: arena_init failed in "
                       "test_alignment_mask_correctness\n");
                return false;
        }

        /* align=1: any address is valid; verify the returned pointer is inside
         * the buffer (basic sanity). */
        st = arena_alloc(&arena, &ptr, 1U, 1U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: alloc align=1 failed\n");
                passed = false;
        }

        /* align=2: address & 1 must be 0 */
        st = arena_alloc(&arena, &ptr, 1U, 2U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: alloc align=2 failed\n");
                passed = false;
        } else {
                addr = (uintptr_t)ptr;
                if ((addr & (2U - 1U)) != 0U) {
                        printf("FAIL: ptr not 2-byte aligned (addr=0x%zx)\n",
                               (size_t)addr);
                        passed = false;
                }
        }

        /* align=4: address & 3 must be 0 */
        st = arena_alloc(&arena, &ptr, 1U, 4U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: alloc align=4 failed\n");
                passed = false;
        } else {
                addr = (uintptr_t)ptr;
                if ((addr & (4U - 1U)) != 0U) {
                        printf("FAIL: ptr not 4-byte aligned (addr=0x%zx)\n",
                               (size_t)addr);
                        passed = false;
                }
        }

        /* align=8: address & 7 must be 0 */
        st = arena_alloc(&arena, &ptr, 1U, 8U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: alloc align=8 failed\n");
                passed = false;
        } else {
                addr = (uintptr_t)ptr;
                if ((addr & (8U - 1U)) != 0U) {
                        printf("FAIL: ptr not 8-byte aligned (addr=0x%zx)\n",
                               (size_t)addr);
                        passed = false;
                }
        }

        /* align=16: address & 15 must be 0 */
        st = arena_alloc(&arena, &ptr, 1U, 16U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: alloc align=16 failed\n");
                passed = false;
        } else {
                addr = (uintptr_t)ptr;
                if ((addr & (16U - 1U)) != 0U) {
                        printf("FAIL: ptr not 16-byte aligned (addr=0x%zx)\n",
                               (size_t)addr);
                        passed = false;
                }
        }

        if (passed) {
                printf("PASS: test_alignment_mask_correctness\n");
        }
        return passed;
}

int
main(void)
{
        static uint8_t buffer[DEMO_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        void *p1, *p2, *p3;
        arena_marker_t mk;
        size_t used, high, cap;
        bool all_passed = true;

        /* Run additional tests */
        all_passed = test_invalid_alignment() && all_passed;
        all_passed = test_zero_size_allocation() && all_passed;
        all_passed = test_null_pointers() && all_passed;
        all_passed = test_alignment_boundaries() && all_passed;
        all_passed = test_invalid_argument() && all_passed;
        all_passed = test_alloc_exact_capacity() && all_passed;
        all_passed = test_get_used_transitions() && all_passed;
        all_passed = test_marker_null_arena() && all_passed;
        all_passed = test_alignment_mask_correctness() && all_passed;

        if (!all_passed) {
                printf("Some tests failed!\n");
                return EXIT_FAILURE;
        }

        /* Original demo code */
        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf("arena_init failed!\n");
                return EXIT_FAILURE;
        }

        cap = arena_get_capacity(&arena);
        printf("Arena capacity: %zu bytes\n", cap);

        /* Allocate 24 bytes, default alignment (8) */
        st = arena_alloc(&arena, &p1, 24U, 0U);
        if (st != ARENA_STATUS_OK) {
                printf("arena_alloc failed for p1!\n");
                return EXIT_FAILURE;
        }
        printf("p1 = %p (size 24)\n", p1);

        /* Allocate 13 bytes, explicit 16-byte alignment */
        st = arena_alloc(&arena, &p2, 13U, 16U);
        if (st != ARENA_STATUS_OK) {
                printf("arena_alloc failed for p2!\n");
                return EXIT_FAILURE;
        }
        printf("p2 = %p (size 13, align 16)\n", p2);

        /* Record a marker, then allocate a further 40 bytes */
        mk = arena_get_marker(&arena);
        st = arena_alloc(&arena, &p3, 40U, 0U);
        if (st != ARENA_STATUS_OK) {
                printf("arena_alloc failed for p3!\n");
                return EXIT_FAILURE;
        }
        printf("p3 = %p (size 40)\n", p3);

        /* Query usage */
        used = arena_get_used(&arena);
        high = arena_get_high_water(&arena);
        printf("After allocations: used = %zu, high_water = %zu\n", used, high);

        /* Attempt to allocate beyond the limit (should fail) */
        st = arena_alloc(&arena, &p1, 100U, 0U);
        if (st == ARENA_STATUS_OUT_OF_MEMORY) {
                printf("Correctly rejected out-of-memory request.\n");
        } else {
                printf("ERROR: Should have failed out-of-memory!\n");
                return EXIT_FAILURE;
        }

        /* Rewind to marker (release p3) */
        arena_rewind(&arena, mk);
        printf("After rewind to marker: used = %zu\n", arena_get_used(&arena));

        /* Reset arena completely */
        arena_reset(&arena);
        printf("After reset: used = %zu, high_water = %zu\n",
               arena_get_used(&arena), arena_get_high_water(&arena));

        return EXIT_SUCCESS;
}
