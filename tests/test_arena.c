/*********************************************************************
 * @file arena_demo.c
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
#include <stdio.h>
#include <stdbool.h>

#define DEMO_BUF_SIZE (128U)

static bool test_invalid_alignment(void);
static bool test_zero_size_allocation(void);
static bool test_null_pointers(void);
static bool test_alignment_boundaries(void);

static bool test_invalid_alignment(void)
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
                printf("FAIL: Expected ARENA_STATUS_INVALID_ALIGNMENT for alignment=3, got %d\n", st);
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

static bool test_zero_size_allocation(void)
{
        static uint8_t buffer[DEMO_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        void *ptr;
        bool passed = true;

        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: arena_init failed in test_zero_size_allocation\n");
                return false;
        }

        /* Test zero size allocation */
        st = arena_alloc(&arena, &ptr, 0U, 0U);
        if (st != ARENA_STATUS_OUT_OF_MEMORY) {
                printf("FAIL: Expected ARENA_STATUS_OUT_OF_MEMORY for size=0, got %d\n", st);
                passed = false;
        }

        if (passed) {
                printf("PASS: test_zero_size_allocation\n");
        }
        return passed;
}

static bool test_null_pointers(void)
{
        static uint8_t buffer[DEMO_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        void *ptr;
        bool passed = true;

        /* Test NULL arena pointer */
        st = arena_alloc(NULL, &ptr, 16U, 0U);
        if (st != ARENA_STATUS_NULL_POINTER) {
                printf("FAIL: Expected ARENA_STATUS_NULL_POINTER for NULL arena, got %d\n", st);
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
                printf("FAIL: Expected ARENA_STATUS_NULL_POINTER for NULL result, got %d\n", st);
                passed = false;
        }

        /* Test NULL buffer in init */
        st = arena_init(&arena, NULL, sizeof(buffer));
        if (st != ARENA_STATUS_NULL_POINTER) {
                printf("FAIL: Expected ARENA_STATUS_NULL_POINTER for NULL buffer, got %d\n", st);
                passed = false;
        }

        if (passed) {
                printf("PASS: test_null_pointers\n");
        }
        return passed;
}

static bool test_alignment_boundaries(void)
{
        static uint8_t buffer[DEMO_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        void *ptr1, *ptr2, *ptr3, *ptr4;
        bool passed = true;

        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: arena_init failed in test_alignment_boundaries\n");
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

        /* Verify alignments */
        if (((uintptr_t)ptr1 & 1U) != 0U) {
                printf("FAIL: ptr1 not aligned to 1\n");
                passed = false;
        }
        if (((uintptr_t)ptr2 & 1U) != 0U) {
                printf("FAIL: ptr2 not aligned to 2\n");
                passed = false;
        }
        if (((uintptr_t)ptr3 & 3U) != 0U) {
                printf("FAIL: ptr3 not aligned to 4\n");
                passed = false;
        }
        if (((uintptr_t)ptr4 & 7U) != 0U) {
                printf("FAIL: ptr4 not aligned to 8\n");
                passed = false;
        }

        if (passed) {
                printf("PASS: test_alignment_boundaries\n");
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

        if (!all_passed) {
                printf("Some tests failed!\n");
                return -1;
        }

        /* Original demo code */
        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf("arena_init failed!\n");
                return -1;
        }

        cap = arena_get_capacity(&arena);
        printf("Arena capacity: %zu bytes\n", cap);

        /* Allocate 24 bytes, default alignment (8) */
        st = arena_alloc(&arena, &p1, 24U, 0U);
        if (st != ARENA_STATUS_OK) {
                printf("arena_alloc failed for p1!\n");
                return -1;
        }
        printf("p1 = %p (size 24)\n", p1);

        /* Allocate 13 bytes, explicit 16-byte alignment */
        st = arena_alloc(&arena, &p2, 13U, 16U);
        if (st != ARENA_STATUS_OK) {
                printf("arena_alloc failed for p2!\n");
                return -1;
        }
        printf("p2 = %p (size 13, align 16)\n", p2);

        /* Record a marker, then allocate a further 40 bytes */
        mk = arena_get_marker(&arena);
        st = arena_alloc(&arena, &p3, 40U, 0U);
        if (st != ARENA_STATUS_OK) {
                printf("arena_alloc failed for p3!\n");
                return -1;
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
                return -1;
        }

        /* Rewind to marker (release p3) */
        arena_rewind(&arena, mk);
        printf("After rewind to marker: used = %zu\n", arena_get_used(&arena));

        /* Reset arena completely */
        arena_reset(&arena);
        printf("After reset: used = %zu, high_water = %zu\n",
               arena_get_used(&arena), arena_get_high_water(&arena));

        return 0;
}
