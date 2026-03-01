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

#include <stdio.h>
#include "arena.h"

#define DEMO_BUF_SIZE  (128U)

int main(void)
{
    static uint8_t buffer[DEMO_BUF_SIZE];
    arena_t arena;
    arena_status_t st;
    void *p1, *p2, *p3;
    arena_marker_t mk;
    size_t used, high, cap;

    /* Initialise arena */
    st = arena_init(&arena, buffer, sizeof(buffer));
    if (st != ARENA_STATUS_OK) {
        printf("arena_init failed!\n");
        return -1;
    }

    cap = arena_get_capacity(&arena);
    printf("Arena capacity: %zu bytes\n", cap);

    /* Allocate 24 bytes, default alignment (8) */
    p1 = arena_alloc(&arena, 24U, 0U);
    printf("p1 = %p (size 24)\n", p1);

    /* Allocate 13 bytes, explicit 16-byte alignment */
    p2 = arena_alloc(&arena, 13U, 16U);
    printf("p2 = %p (size 13, align 16)\n", p2);

    /* Record a marker, then allocate a further 40 bytes */
    mk = arena_get_marker(&arena);
    p3 = arena_alloc(&arena, 40U, 0U);
    printf("p3 = %p (size 40)\n", p3);

    /* Query usage */
    used = arena_get_used(&arena);
    high = arena_get_high_water(&arena);
    printf("After allocations: used = %zu, high_water = %zu\n", used, high);

    /* Attempt to allocate beyond the limit (should fail) */
    if (arena_alloc(&arena, 100U, 0U) == NULL) {
        printf("Correctly rejected out-of-memory request.\n");
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
