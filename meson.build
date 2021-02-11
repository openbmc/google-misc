project(
  'google-misc',
  'c',
  version: '0.1',
  meson_version: '>=0.53.2',
  default_options: [
    'warning_level=3',
    'werror=true',
    'cpp_std=c++17',
    'c_std=c18',
    'tests=' + (meson.is_subproject() ? 'disabled' : 'auto'),
  ])

# Dummy file to make code coverage happy
executable('hello', 'hello.c')

# Meson has no feature option to str...
tests_str = get_option('tests').enabled() ? 'enabled' : 'disabled'
if get_option('tests').auto()
  tests_str = 'auto'
endif

subproject('ncsid', default_options: 'tests=' + tests_str)
subproject('metrics-ipmi-blobs', default_options: 'tests=' + tests_str)
