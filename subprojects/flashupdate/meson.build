project('flashupdate', ['cpp'],
  version: '0.1', meson_version: '>=0.57.0',
  default_options: [
    'warning_level=2',
    'werror=true',
    'cpp_std=c++20',
    'tests=' + (meson.is_subproject() ? 'disabled' : 'auto'),
  ]
)

subdir('include')
subdir('internal')
subdir('src')

if not get_option('tests').disabled()
  subdir('test')
endif