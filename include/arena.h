/*********************************************************************
 * @file arena.h
 * @brief Deterministic arena (bump) allocator.
 *
 * This allocator gives you a *static* memory pool that can be used in a
 * safety critical environment.  Allocation is O(1) and never fails
 * (except when the pool is exhausted).  Deallocation is performed by a
 * whole arena reset or by rewinding to a previously saved checkpoint.
 *
 * The implementation follows the MISRA C:2012 guidelines and can
 * therefore be used as a building block for IEC 61508 SIL 2/3
 * applications.
 *
 *********************************************************************/

/* MISRA C:2012 Dir 1.1: use standard include guard instead of #pragma once */
#ifndef ARENA_H_
#define ARENA_H_

/* ------------------------------------------------------------------ */
/*   INCLUDES                                                         */
/* ------------------------------------------------------------------ */
#include "arena_cfg.h"
#include <stddef.h> /* size_t, NULL   */
#include <stdint.h> /* uintptr_t      */

/* ------------------------------------------------------------------ */
/*   Doxygen group                                                    */
/* ------------------------------------------------------------------ */
/**
 * @defgroup arena Deterministic arena allocator
 * @{
 */

/* ------------------------------------------------------------------ */
/*   ARENA TYPE                                                        */
/* ------------------------------------------------------------------ */

/**
 * @brief Arena storage and state.
 *
 * This type is intentionally defined in the public header so callers can
 * allocate it statically or on the stack without dynamic memory.
 *
 * The fields are internal allocator state and must not be modified
 * directly by user code.
 */
typedef struct arena_s {
        uint8_t *start;        /**< Start of the backing buffer           */
        size_t capacity;       /**< Total buffer size in bytes            */
        size_t current_offset; /**< Next free offset from @ref start      */
        size_t high_water;     /**< Maximum observed used size            */
        uint8_t poison;        /**< Debug poison pattern cache            */
} arena_t;

/**
 * @brief Marker used to remember an arena position (checkpoint).
 *
 * The value is the byte offset from the start of the arena buffer.
 * The sentinel value @ref ARENA_MARKER_INVALID is returned by
 * arena_get_marker() when a NULL arena pointer is supplied.
 */
typedef size_t arena_marker_t;

/**
 * @brief Sentinel returned by arena_get_marker() when @p arena is NULL.
 *
 * A valid marker is always in the range [0, capacity].  SIZE_MAX can
 * never be a valid in-bounds offset, so it is safe to use as an error
 * indicator without adding a new return type.
 *
 * Callers should compare the return value against ARENA_MARKER_INVALID
 * before using the marker.
 */
#define ARENA_MARKER_INVALID ((arena_marker_t)(SIZE_MAX))

/* ------------------------------------------------------------------ */
/*   RETURN STATUS                                                    */
/* ------------------------------------------------------------------ */

/**
 * @brief Status values returned by the arena API.
 *
 * All public functions returning a status use this enumeration.
 */
typedef enum {
        ARENA_STATUS_OK = 0, /**< Operation completed successfully */
        ARENA_STATUS_NULL_POINTER =
            1, /**< A required pointer argument was NULL */
        ARENA_STATUS_OUT_OF_MEMORY =
            2, /**< Not enough space left in the arena */
        ARENA_STATUS_INVALID_ALIGNMENT =
            3, /**< Alignment is not a power of two or is zero */
        ARENA_STATUS_INVALID_ARGUMENT = 4 /**< size or buffer length is zero */
} arena_status_t;

/* ------------------------------------------------------------------ */
/*   PUBLIC FUNCTION PROTOTYPES                                        */
/* ------------------------------------------------------------------ */

/**
 * @brief Initialise an arena instance.
 *
 * The caller must provide a storage buffer that lives at least as long as
 * the arena.  The buffer may be a static array, a linker section, or a
 * memory region supplied by the bootloader.
 *
 * @param[in,out] arena   Pointer to an arena object (must not be NULL)
 * @param[in]     buffer  Pointer to the raw memory buffer (must not be NULL)
 * @param[in]     size    Size of the buffer in bytes (must be > 0)
 *
 * @retval ARENA_STATUS_OK               Initialisation succeeded.
 * @retval ARENA_STATUS_NULL_POINTER     @p arena or @p buffer is NULL.
 * @retval ARENA_STATUS_INVALID_ARGUMENT @p size is zero, or
 *                                       @p size > (UINTPTR_MAX -
 * (uintptr_t)buffer) (addition would overflow the pointer).
 *
 * @pre arena != NULL
 * @pre buffer != NULL
 * @pre size > 0
 * @pre size <= (UINTPTR_MAX - (uintptr_t)buffer)
 *
 * @post The arena is ready for allocations.  Its used size is 0.
 */
extern arena_status_t arena_init(arena_t *const arena, void *const buffer,
                                 size_t size);

/**
 * @brief Allocate a block of memory from the arena.
 *
 * The function returns a pointer that is suitably aligned for the
 * requested alignment.  If @p alignment is 0 the default alignment
 * ( @ref ARENA_CFG_DEFAULT_ALIGNMENT ) is used.
 *
 * @param[in,out] arena      Pointer to an already initialised arena (must not
 * be NULL)
 * @param[out]    result     Pointer to store the allocated memory address
 * @param[in]     size       Number of bytes to allocate (must be > 0)
 * @param[in]     alignment  Desired alignment in bytes (must be a power of two,
 *                           0 means default alignment)
 *
 * @retval ARENA_STATUS_OK               Allocation succeeded; @p *result is
 * valid.
 * @retval ARENA_STATUS_NULL_POINTER     @p arena or @p result is NULL.
 * @retval ARENA_STATUS_INVALID_ALIGNMENT @p alignment is non-zero and not a
 * power of two.
 * @retval ARENA_STATUS_INVALID_ARGUMENT @p size is zero.
 * @retval ARENA_STATUS_OUT_OF_MEMORY    Insufficient contiguous space remains.
 *
 * @pre arena != NULL
 * @pre result != NULL
 * @pre size > 0
 * @pre alignment == 0 || is_power_of_two(alignment)
 *
 * @post If successful, *result points to allocated memory
 * @post If the function fails and @p result is non-NULL, *result is set to NULL
 * @post The arena used size is increased by the allocated size
 *
 * @note The caller must never free the returned pointer individually.
 */
extern arena_status_t arena_alloc(arena_t *const arena, void **const result,
                                  size_t size, size_t alignment);

/**
 * @brief Allocate a zero-initialised block of memory from the arena.
 *
 * This is identical to @ref arena_alloc(), except the returned block is
 * filled with zero bytes on success.
 *
 * @param[in,out] arena      Pointer to an already initialised arena (must not
 *                           be NULL)
 * @param[out]    result     Pointer to store the allocated memory address
 * @param[in]     size       Number of bytes to allocate (must be > 0)
 * @param[in]     alignment  Desired alignment in bytes (must be a power of two,
 *                           0 means default alignment)
 *
 * @retval ARENA_STATUS_OK                Allocation succeeded; @p *result is
 *                                       valid and points to zeroed memory.
 * @retval ARENA_STATUS_NULL_POINTER      @p arena or @p result is NULL.
 * @retval ARENA_STATUS_INVALID_ALIGNMENT @p alignment is non-zero and not a
 *                                       power of two.
 * @retval ARENA_STATUS_INVALID_ARGUMENT  @p size is zero.
 * @retval ARENA_STATUS_OUT_OF_MEMORY     Insufficient contiguous space remains.
 */
extern arena_status_t arena_alloc_zero(arena_t *const arena,
                                       void **const result, size_t size,
                                       size_t alignment);

/**
 * @brief Allocate an array from the arena using element count semantics.
 *
 * This helper computes the total byte size as @p element_size * @p count and
 * rejects overflow before delegating to @ref arena_alloc().
 *
 * @param[in,out] arena         Pointer to an already initialised arena
 * @param[out]    result        Pointer to store the allocated memory address
 * @param[in]     element_size  Size of each element in bytes (must be > 0)
 * @param[in]     alignment     Desired alignment in bytes (must be a power of
 *                              two, 0 means default alignment)
 * @param[in]     count         Number of elements to allocate (must be > 0)
 *
 * @retval ARENA_STATUS_OK                Allocation succeeded.
 * @retval ARENA_STATUS_NULL_POINTER      @p arena or @p result is NULL.
 * @retval ARENA_STATUS_INVALID_ALIGNMENT @p alignment is non-zero and not a
 *                                       power of two.
 * @retval ARENA_STATUS_INVALID_ARGUMENT  @p element_size is zero, @p count is
 *                                       zero, or @p element_size * @p count
 *                                       would overflow @c size_t.
 * @retval ARENA_STATUS_OUT_OF_MEMORY     Insufficient contiguous space remains.
 */
extern arena_status_t arena_alloc_array(arena_t *const arena,
                                        void **const result,
                                        size_t element_size, size_t alignment,
                                        size_t count);

/**
 * @brief Allocate a zero-initialised array from the arena.
 *
 * This is identical to @ref arena_alloc_array(), except the returned block is
 * filled with zero bytes on success.
 *
 * @param[in,out] arena         Pointer to an already initialised arena
 * @param[out]    result        Pointer to store the allocated memory address
 * @param[in]     element_size  Size of each element in bytes (must be > 0)
 * @param[in]     alignment     Desired alignment in bytes (must be a power of
 *                              two, 0 means default alignment)
 * @param[in]     count         Number of elements to allocate (must be > 0)
 *
 * @retval ARENA_STATUS_OK                Allocation succeeded and memory is
 *                                       zero-initialised.
 * @retval ARENA_STATUS_NULL_POINTER      @p arena or @p result is NULL.
 * @retval ARENA_STATUS_INVALID_ALIGNMENT @p alignment is non-zero and not a
 *                                       power of two.
 * @retval ARENA_STATUS_INVALID_ARGUMENT  @p element_size is zero, @p count is
 *                                       zero, or @p element_size * @p count
 *                                       would overflow @c size_t.
 * @retval ARENA_STATUS_OUT_OF_MEMORY     Insufficient contiguous space remains.
 */
extern arena_status_t arena_alloc_array_zero(arena_t *const arena,
                                             void **const result,
                                             size_t element_size,
                                             size_t alignment, size_t count);

/**
 * @brief Store the current arena position for later rewinding.
 *
 * The returned marker can later be passed to arena_rewind() to restore the
 * arena to exactly this point.  This is the recommended way to create
 * function-scoped temporary allocations: save a marker on entry, allocate
 * temporary working memory, then rewind before returning.
 *
 * @param[in] arena  Pointer to a valid arena (must not be NULL)
 *
 * @return Marker representing the current byte offset from the buffer
 *         start, or @ref ARENA_MARKER_INVALID if @p arena is NULL.
 *
 * @pre arena != NULL
 *
 * @post Returned marker is valid for use with arena_rewind() iff
 *       the return value != ARENA_MARKER_INVALID.
 */
extern arena_marker_t arena_get_marker(const arena_t *const arena);

/**
 * @brief Rewind the arena to a previously saved marker.
 *
 * All memory allocated after the supplied marker becomes available again.
 * If the marker is invalid (greater than the used size) the function has
 * no effect.
 *
 * @param[in,out] arena  Pointer to an arena (must not be NULL)
 * @param[in]     marker Marker obtained from arena_get_marker().
 *
 * @pre arena != NULL
 *
 * @post The arena used size is equal to the supplied marker (if marker was
 * valid)
 */
extern void arena_rewind(arena_t *const arena, arena_marker_t marker);

/**
 * @brief Reset the arena to its initial empty state.
 *
 * The whole buffer becomes available again.  If @ref
 * ARENA_CFG_DEBUG_POISON is enabled the memory region is filled with the
 * poison pattern.
 *
 * @param[in,out] arena  Pointer to an arena (must not be NULL)
 *
 * @pre arena != NULL
 *
 * @post Used size of the arena is 0.
 */
extern void arena_reset(arena_t *const arena);

/**
 * @brief Query the amount of memory currently used.
 *
 * @param[in] arena  Pointer to a valid arena (must not be NULL)
 *
 * @return Number of bytes allocated since the last reset (or init).
 *
 * @pre arena != NULL
 */
extern size_t arena_get_used(const arena_t *const arena);

/**
 * @brief Query the high-water mark (maximum ever used) for the arena.
 *
 * The high-water mark is never cleared by @ref arena_reset(),
 * enabling you to verify worst-case memory consumption.
 *
 * @param[in] arena  Pointer to a valid arena (must not be NULL)
 *
 * @return Highest number of bytes used by the arena at any point.
 *
 * @pre arena != NULL
 */
extern size_t arena_get_high_water(const arena_t *const arena);

/**
 * @brief Query the total capacity of the arena.
 *
 * @param[in] arena  Pointer to a valid arena (must not be NULL)
 *
 * @return Total size (in bytes) of the backing buffer.
 *
 * @pre arena != NULL
 */
extern size_t arena_get_capacity(const arena_t *const arena);

/**
 * @brief Allocate one typed object and store the pointer in a typed variable.
 *
 * Pass the address of a typed pointer variable, for example
 * `my_type_t *ptr = NULL;` followed by
 * `ARENA_ALLOC_OBJECT(&arena, &ptr, my_type_t);`.
 */
#define ARENA_ALLOC_OBJECT(arena, result, type)                                \
        arena_alloc((arena), (void **)(result), sizeof(type), _Alignof(type))

/**
 * @brief Allocate an array of typed objects and store the pointer.
 */
#define ARENA_ALLOC_ARRAY(arena, result, type, count)                          \
        arena_alloc_array((arena), (void **)(result), sizeof(type),            \
                          _Alignof(type), (count))

/**
 * @brief Allocate one zero-initialised typed object and store the pointer.
 */
#define ARENA_ALLOC_OBJECT_ZERO(arena, result, type)                           \
        arena_alloc_zero((arena), (void **)(result), sizeof(type),             \
                         _Alignof(type))

/**
 * @brief Allocate a zero-initialised array of typed objects and store the
 * pointer.
 */
#define ARENA_ALLOC_ARRAY_ZERO(arena, result, type, count)                     \
        arena_alloc_array_zero((arena), (void **)(result), sizeof(type),       \
                               _Alignof(type), (count))

/**
 * @}
 */  /* end of group arena */

#endif /* ARENA_H_ */
