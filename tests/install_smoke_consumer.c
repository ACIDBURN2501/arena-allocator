/*********************************************************************
 * @file install_smoke_consumer.c
 * @brief Minimal consumer used to validate staged installs.
 *********************************************************************/

#include <arena.h>
#include <stdint.h>

int
main(void)
{
        static uint8_t buffer[64U];
        arena_t arena;
        arena_status_t st;
        void *ptr = (void *)0;

        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                return 1;
        }

        st = arena_alloc(&arena, &ptr, 16U, 0U);
        if (st != ARENA_STATUS_OK) {
                return 2;
        }

        if ((ptr == NULL) || (arena_get_capacity(&arena) != sizeof(buffer))) {
                return 3;
        }

        return 0;
}
