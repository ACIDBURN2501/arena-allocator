# arena-allocator

[![CI](https://github.com/ACIDBURN2501/arena-allocator/actions/workflows/ci.yml/badge.svg)](https://github.com/ACIDBURN2501/arena-allocator/actions/workflows/ci.yml)

A deterministic arena (bump) allocator in C for safety-critical embedded systems.

## Features

- **No dynamic memory** — fixed-size static buffers; no `malloc` / `free`.
- **Deterministic allocation** — O(1) allocation with no branching or overhead.
- **Checkpointing** — save and restore arena state with markers for scoped allocations.
- **High-water tracking** — monitor maximum memory usage for static analysis.
- **MISRA C:2012 compliant** — suitable for IEC 61508 SIL 2/3 environments.
- **Debug poisoning** — optional fill with known patterns to detect use-after-free.

## Installation

### Copy-in (recommended for embedded targets)

Copy three files into your project tree -- no build system required:

```
include/arena.h
include/arena_cfg.h
src/arena.c
```

Place them in the same directory, then include the header:

```c
#include "arena.h"
```

### Meson subproject

Add this repo as a wrap dependency or subproject. The library exposes an
`arena_dep` dependency object that carries the correct include path:

```meson
arena_dep = dependency('arena-allocator', fallback : ['arena-allocator', 'arena_dep'])
```

## Quick Start

```c
#include <stdio.h>
#include "arena.h"

int main(void)
{
        static uint8_t buffer[1024];
        arena_t arena;
        void *p1, *p2, *p3;
        arena_status_t st;
        arena_marker_t marker;

        /* Initialize arena */
        st = arena_init(&arena, buffer, sizeof(buffer));
        if (st != ARENA_STATUS_OK) {
                return -1;
        }

        /* Allocate memory */
        st = arena_alloc(&arena, &p1, 100U, 0U);
        st = arena_alloc(&arena, &p2, 200U, 16U);

        /* Save checkpoint */
        marker = arena_get_marker(&arena);

        /* Allocate more */
        st = arena_alloc(&arena, &p3, 300U, 0U);

        /* Rewind to checkpoint (releases p3) */
        arena_rewind(&arena, marker);

        /* Reset completely */
        arena_reset(&arena);

        return 0;
}
```

## Configuration

All macros live in `arena_cfg.h` and can be overridden before including the
header or passed as `-D` flags on the compiler command line.

| Macro | Description | Default |
|---|---|---|
| `ARENA_CFG_DEFAULT_ALIGNMENT` | Default alignment when 0 is passed to `arena_alloc()`. Must be a power of two. | `8U` |
| `ARENA_CFG_DEBUG_POISON` | Fill memory with poison pattern on reset/rewind (1=enabled, 0=disabled). | `0U` |
| `ARENA_CFG_POISON_PATTERN` | Byte pattern used for debug poisoning. | `0xAAU` |

## Building

```sh
# Library only (release)
meson setup build --buildtype=release
meson compile -C build

# With unit tests
meson setup build --buildtype=debug -Dbuild_tests=true
meson compile -C build
meson test -C build
```

## Notes

| Topic | Note |
|---|---|
| **Memory** | All storage is static. The backing buffer must outlive the arena instance. |
| **Thread safety** | Not thread-safe. Protect with an external mutex if needed. |
| **Deallocation** | Memory is freed deterministically via `arena_rewind()` or `arena_reset()`. |
| **Error handling** | All functions return status codes; no `errno` or exceptions. |
| **WCET** | All operations have deterministic O(1) worst-case execution time. |
| **Alignment** | Custom alignments must be powers of two; verified at compile time. |

## API Reference

### Status codes

All functions that can fail return an `arena_status_t`:

| Value | Name | Meaning |
|---|---|---|
| `0` | `ARENA_STATUS_OK` | Operation succeeded |
| `1` | `ARENA_STATUS_NULL_POINTER` | A required pointer argument was NULL |
| `2` | `ARENA_STATUS_OUT_OF_MEMORY` | Not enough space left in the arena |
| `3` | `ARENA_STATUS_INVALID_ALIGNMENT` | Alignment is non-zero and not a power of two |
| `4` | `ARENA_STATUS_INVALID_ARGUMENT` | `size` or buffer length argument is zero |

### Initialization

```c
arena_status_t arena_init(arena_t *arena, void *buffer, size_t size);
```

Initialize an arena with a backing buffer. The buffer must remain valid for the
arena's lifetime. Returns `ARENA_STATUS_INVALID_ARGUMENT` if `size` is zero,
`ARENA_STATUS_NULL_POINTER` if either pointer is NULL.

### Allocation

```c
arena_status_t arena_alloc(arena_t *arena, void **result, size_t size, size_t alignment);
```

Allocate `size` bytes with the requested alignment. Pass `alignment = 0` to use
the default (`ARENA_CFG_DEFAULT_ALIGNMENT`, 8 bytes). Returns
`ARENA_STATUS_INVALID_ARGUMENT` if `size` is zero,
`ARENA_STATUS_INVALID_ALIGNMENT` if `alignment` is non-zero and not a power of
two, or `ARENA_STATUS_OUT_OF_MEMORY` if the arena is full. On failure,
`*result` is set to `NULL`.

### Checkpointing

```c
arena_marker_t arena_get_marker(const arena_t *arena);
void arena_rewind(arena_t *arena, arena_marker_t marker);
```

Save and restore arena state for scoped allocations.

### Query Functions

```c
size_t arena_get_used(const arena_t *arena);
size_t arena_get_high_water(const arena_t *arena);
size_t arena_get_capacity(const arena_t *arena);
```

Query current usage, maximum usage, and total capacity.

### Reset

```c
void arena_reset(arena_t *arena);
```

Reset the arena to its initial empty state.
