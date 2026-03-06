/*********************************************************************
 * @file arena_cfg.h
 * @brief Public compile-time configuration for the deterministic arena.
 *
 * The values defined here can be overridden on the compiler command line
 * (e.g. -DARENA_CFG_DEFAULT_ALIGNMENT=16).
 *
 * @note This file is deliberately tiny to keep the library usable in a
 *       *no OS* environment typical for safety critical targets.
 *********************************************************************/

#ifndef ARENA_CFG_H_
#define ARENA_CFG_H_

/**
 * @def ARENA_CFG_DEFAULT_ALIGNMENT
 * @brief Alignment (in bytes) used when the caller passes 0 as the
 *        alignment argument to arena_alloc().
 *
 * Must be a power of two and at least 1.
 */
#ifndef ARENA_CFG_DEFAULT_ALIGNMENT
#define ARENA_CFG_DEFAULT_ALIGNMENT (8U)
#endif

/**
 * @def ARENA_CFG_DEBUG_POISON
 * @brief When set to 1, the whole buffer is filled with a known pattern
 *        on reset / rewind. This is invaluable for finding use-after-
 *        free bugs in unit tests, but should be disabled on final
 *        production builds.
 */
#ifndef ARENA_CFG_DEBUG_POISON
#define ARENA_CFG_DEBUG_POISON (0U)
#endif

/**
 * @def ARENA_CFG_POISON_PATTERN
 * @brief Byte pattern used when @ref ARENA_CFG_DEBUG_POISON is enabled.
 */
#ifndef ARENA_CFG_POISON_PATTERN
#define ARENA_CFG_POISON_PATTERN (0xAAU)
#endif

#endif /* ARENA_CFG_H_ */
