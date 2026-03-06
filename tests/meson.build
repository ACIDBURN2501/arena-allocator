inc = include_directories('../include', '../config')

test_exe = executable('test_arena',
        'test_arena.c',
        'test_arena_api.c',
        'test_arena_allocation.c',
        'test_arena_markers.c',
        '../src/arena.c',
        include_directories : inc,
        c_args : ['-DTEST_BUILD'])

poison_test_exe = executable('test_arena_poison',
        'test_arena_poison.c', '../src/arena.c',
        include_directories : inc,
        c_args : [
                '-DTEST_BUILD',
                '-DARENA_CFG_DEBUG_POISON=1U',
                '-DARENA_CFG_POISON_PATTERN=0xDEU',
        ])

test('arena_test', test_exe)
test('arena_poison_test', poison_test_exe)
