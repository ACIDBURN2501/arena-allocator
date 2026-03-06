project(
  'arena-allocator',
  'c',
  version: '1.1.6',
  default_options: ['warning_level=3'],
)

inc = include_directories('include', 'config')
src = ['src/arena.c']

libarena = static_library('arena', src, include_directories: inc, install: true)

install_headers('include/arena.h')

if get_option('build_tests')
  subdir('tests')
endif
