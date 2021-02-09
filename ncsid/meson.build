project(
  'gbmc-ncsid',
  ['cpp', 'c'],
  version: '0.1',
  meson_version: '>=0.53.2',
  default_options: [
    'warning_level=3',
    'werror=true',
    'cpp_std=c++17',
    'c_std=c11',
    'tests=' + (meson.is_subproject() ? 'disabled' : 'auto'),
  ])

subdir('src')

build_tests = get_option('tests')

if not build_tests.disabled()
  subdir('test')
endif
