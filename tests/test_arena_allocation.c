/*********************************************************************
 * @file test_arena_allocation.c
 * @brief Allocation and capacity tests for the arena allocator.
 *********************************************************************/

#include "test_arena.h"

static bool test_alignment_boundaries(void);
static bool test_alloc_exact_capacity(void);
static bool test_alignment_mask_correctness(void);
static bool test_init_size_overflow(void);

static bool
test_alignment_boundaries(void)
{
        static uint8_t buffer[DEMO_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        void *ptr1;
        void *ptr2;
        void *ptr3;
        void *ptr4;
        bool passed = true;

        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: arena_init failed in test_alignment_boundaries\n");
                return false;
        }

        st = arena_alloc(&arena, &ptr1, 8U, 1U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: Alignment=1 failed\n");
                passed = false;
        }

        st = arena_alloc(&arena, &ptr2, 8U, 2U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: Alignment=2 failed\n");
                passed = false;
        }

        st = arena_alloc(&arena, &ptr3, 8U, 4U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: Alignment=4 failed\n");
                passed = false;
        }

        st = arena_alloc(&arena, &ptr4, 8U, 8U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: Alignment=8 failed\n");
                passed = false;
        }

        if (((uintptr_t)ptr1 & 0U) != 0U) {
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

static bool
test_alloc_exact_capacity(void)
{
        static uint8_t buffer[DEMO_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        void *ptr;
        bool passed = true;

        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: arena_init failed in test_alloc_exact_capacity\n");
                return false;
        }

        st = arena_alloc(&arena, &ptr, sizeof(buffer), 1U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: Exact-capacity alloc returned %d\n", st);
                passed = false;
        }

        st = arena_alloc(&arena, &ptr, 1U, 1U);
        if (st != ARENA_STATUS_OUT_OF_MEMORY) {
                printf("FAIL: Expected OUT_OF_MEMORY after full alloc, got %d\n",
                       st);
                passed = false;
        }

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

        st = arena_alloc(&arena, &ptr, 1U, 1U);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: alloc align=1 failed\n");
                passed = false;
        }

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

static bool
test_init_size_overflow(void)
{
        static uint8_t buffer[DEMO_BUF_SIZE];
        arena_t arena;
        arena_status_t st;
        bool passed = true;
        const size_t headroom = (size_t)(UINTPTR_MAX - (uintptr_t)buffer);

        st = arena_init(&arena, buffer, headroom + 1U);
        if (st != ARENA_STATUS_INVALID_ARGUMENT) {
                printf("FAIL: Expected INVALID_ARGUMENT for size=headroom+1, "
                       "got %d\n",
                       st);
                passed = false;
        }

        st = arena_init(&arena, buffer, (size_t)-1);
        if (st != ARENA_STATUS_INVALID_ARGUMENT) {
                printf("FAIL: Expected INVALID_ARGUMENT for size=SIZE_MAX, "
                       "got %d\n",
                       st);
                passed = false;
        }

        st = arena_init(&arena, buffer, headroom);
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: Expected OK for size=headroom, got %d\n", st);
                passed = false;
        }

        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                printf("FAIL: Expected OK for size=sizeof(buffer), got %d\n",
                       st);
                passed = false;
        }

        if (passed) {
                printf("PASS: test_init_size_overflow\n");
        }
        return passed;
}

bool
run_allocation_tests(void)
{
        bool all_passed = true;

        all_passed = test_alignment_boundaries() && all_passed;
        all_passed = test_alloc_exact_capacity() && all_passed;
        all_passed = test_alignment_mask_correctness() && all_passed;
        all_passed = test_init_size_overflow() && all_passed;

        return all_passed;
}
