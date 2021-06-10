project(
  'ec-ipmi-blobs',
  'cpp',
  version: '0.1',
  meson_version: '>=0.51.0',
  default_options: [
    'warning_level=3',
    'cpp_std=c++17',
  ])

ecblob_incs = include_directories('include')

subdir('src')

if not get_option('tests').disabled()
  subdir('test')
endif
