/*********************************************************************
 * @file arena.c
 * @brief Implementation of the deterministic arena allocator.
 *
 * All public functions are defined here, all internal helpers are
 * static (translation unit local).  The code follows MISRA C:2012
 * guidelines: every conversion is explicit, no undefined behaviour,
 * all limits are checked, and the implementation uses only the
 * standard C library sections that are typically allowed in safety
 * critical projects (stddef, stdint).
 *
 * The arena works with a *bump pointer* (current).  Allocation consists
 * of:
 *   1. Align the current pointer up to the requested alignment.
 *   2. Check that the aligned address + size does not exceed the buffer
 *      end.
 *   3. Return the aligned address and bump the current pointer.
 *
 * Deallocation is deterministic: either a full reset or a rewind to a
 * previously saved marker (offset from the start of the buffer).  The
 * implementation keeps track of the high water mark to aid static
 * analysis of worst case memory usage.
 *
 *********************************************************************/

#include "arena.h"
#include <string.h> /* memset (optional, only used when debug poisoning is enabled) */

/* ------------------------------------------------------------------ */
/*   INTERNAL OPAQUE STRUCT DEFINITION                                 */
/* ------------------------------------------------------------------ */
struct arena_s {
        uint8_t *start;    /**< Pointer to the start of the backing buffer   */
        uint8_t *current;  /**< Bump pointer (next free byte)                */
        uint8_t *end;      /**< One past the last valid byte of the buffer    */
        size_t used;       /**< Bytes currently allocated                     */
        size_t high_water; /**< Highest ever value of @a used                */
#if (ARENA_CFG_DEBUG_POISON != 0)
        uint8_t poison; /**< Debug fill byte (default 0xAA)                */
#endif
};

/* ------------------------------------------------------------------ */
/*   STATIC INLINE HELPERS (MISRA-friendly)                            */
/* ------------------------------------------------------------------ */

/**
 * @brief Check if a value is a power of two.
 *
 * Zero is *not* a power of two.  The function is used to validate the
 * @p alignment parameter.
 *
 * @param[in] value   Value to test.
 *
 * @return true if @p value is a power of two, otherwise false.
 */
static inline bool
is_power_of_two(const size_t value)
{
        /* MISRA Rule 12.2: bitwise operations are allowed on unsigned types */
        return ((value != 0U) && ((value & (value - 1U)) == 0U));
}

/**
 * @brief Round an address up to the next multiple of @p alignment.
 *
 * @p alignment must be a power of two.
 *
 * @param[in] address   Original address (as integer).
 * @param[in] alignment Alignment, power of two.
 *
 * @return Aligned address (as integer).
 */
static inline uintptr_t
align_up(const uintptr_t address, const size_t alignment)
{
        const uintptr_t mask = ((uintptr_t)alignment) - 1U;
        return (address + mask) & (~mask);
}

/**
 * @brief Fill a memory range with a constant byte.
 *
 * This helper abstracts the optional debug poisoning to a single place.
 *
 * @param[in] start   Pointer to first byte to fill.
 * @param[in] size    Number of bytes to fill.
 * @param[in] pattern Fill pattern.
 */
static void
arena_fill(void *const start, const size_t size, const uint8_t pattern)
{
#if (ARENA_CFG_DEBUG_POISON != 0)
        uint8_t *p = (uint8_t *)start;
        size_t i;
        for (i = 0U; i < size; ++i) {
                p[i] = pattern;
        }
        (void)pattern; /* silence unused warning when POISON disabled */
#else
        (void)start;
        (void)size;
        (void)pattern;
        /* No operation in production builds */
#endif
}

/* ------------------------------------------------------------------ */
/*   PUBLIC API IMPLEMENTATION                                          */
/* ------------------------------------------------------------------ */

arena_status_t
arena_init(arena_t *const arena, void *const buffer, const size_t size)
{
        if (arena == NULL) {
                return ARENA_STATUS_NULL_POINTER;
        }
        if (buffer == NULL) {
                return ARENA_STATUS_NULL_POINTER;
        }
        if (size == 0U) {
                return ARENA_STATUS_OUT_OF_MEMORY;
        }

        /* Cast buffer to unsigned byte pointer for arithmetic */
        arena->start = (uint8_t *)buffer;
        arena->current = arena->start;
        arena->end = arena->start + size;
        arena->used = 0U;
        arena->high_water = 0U;
#if (ARENA_CFG_DEBUG_POISON != 0)
        arena->poison = ARENA_CFG_POISON_PATTERN;
#endif

        /* Optional fill with poison to catch use-after-reset in tests */
        arena_fill(arena->start, size, ARENA_CFG_POISON_PATTERN);

        return ARENA_STATUS_OK;
}

/* --------------------------------------------------------------- */
void *
arena_alloc(arena_t *const arena, const size_t size, size_t alignment)
{
        uintptr_t aligned_addr;
        size_t padding;
        uintptr_t cur_addr;
        uintptr_t end_addr;

        /* Defensive checks - return NULL to signal failure */
        if (arena == NULL) {
                return NULL;
        }
        if (size == 0U) {
                return NULL;
        }

        /* Resolve default alignment if caller passes 0 */
        if (alignment == 0U) {
                alignment = ARENA_CFG_DEFAULT_ALIGNMENT;
        }

        /* Alignment must be a power of two (MISRA rule 10.5: explicit cast) */
        if (!is_power_of_two(alignment)) {
                return NULL;
        }

        cur_addr = (uintptr_t)arena->current;
        aligned_addr = align_up(cur_addr, alignment);
        padding = aligned_addr - cur_addr;

        /* Check for overflow of the address range */
        if ((aligned_addr + size) > (uintptr_t)arena->end) {
                /* Out of memory */
                return NULL;
        }

        /* Update internal state */
        arena->current = (uint8_t *)(aligned_addr + size);
        arena->used = (size_t)(arena->current - arena->start);
        if (arena->used > arena->high_water) {
                arena->high_water = arena->used;
        }

        /* Return the aligned address as a void pointer */
        return (void *)aligned_addr;
}

/* --------------------------------------------------------------- */
arena_marker_t
arena_get_marker(const arena_t *const arena)
{
        arena_marker_t marker = 0U;
        if (arena != NULL) {
                marker = (size_t)(arena->current - arena->start);
        }
        return marker;
}

/* --------------------------------------------------------------- */
void
arena_rewind(arena_t *const arena, const arena_marker_t marker)
{
        if (arena == NULL) {
                return;
        }

        /* Marker must be within the used range; if not we ignore it */
        if (marker <= (arena_marker_t)(arena->current - arena->start)) {
                arena->current = arena->start + marker;
                arena->used = marker;

#if (ARENA_CFG_DEBUG_POISON != 0)
                /* Poison the released area to detect stray accesses */
                arena_fill(arena->current,
                           (size_t)(arena->end - arena->current),
                           arena->poison);
#endif
        }
}

/* --------------------------------------------------------------- */
void
arena_reset(arena_t *const arena)
{
        if (arena == NULL) {
                return;
        }

        arena->current = arena->start;
        arena->used = 0U;

#if (ARENA_CFG_DEBUG_POISON != 0)
        arena_fill(arena->start, (size_t)(arena->end - arena->start),
                   arena->poison);
#endif
}

/* --------------------------------------------------------------- */
size_t
arena_get_used(const arena_t *const arena)
{
        return (arena != NULL) ? arena->used : 0U;
}

/* --------------------------------------------------------------- */
size_t
arena_get_high_water(const arena_t *const arena)
{
        return (arena != NULL) ? arena->high_water : 0U;
}

/* --------------------------------------------------------------- */
size_t
arena_get_capacity(const arena_t *const arena)
{
        size_t capacity = 0U;
        if (arena != NULL) {
                capacity = (size_t)(arena->end - arena->start);
        }
        return capacity;
}

/* End of arena.c */
