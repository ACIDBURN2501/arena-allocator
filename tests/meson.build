inc = include_directories('../include', '../config')
python = import('python').find_installation('python3')

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

# The smoke test installs libarena.a and links it from a plain consumer that carries no sanitizer or coverage flags. Instrumented static archives embed sanitizer/gcov runtime symbols that the consumer linker cannot resolve, so the install-smoke test only runs for plain builds.
if get_option('b_sanitize') == 'none' and not get_option('b_coverage')
  test('arena_install_smoke', python,
       args : [files('run_install_smoke.py'), meson.project_source_root(),
               meson.project_build_root()])
endif
