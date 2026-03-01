inc = include_directories('../include', '../config')

test_exe = executable('test_arena',
        'test_arena.c', '../src/arena.c',
        include_directories : inc,
        c_args : ['-DTEST_BUILD'])

test('arena_test', test_exe)
