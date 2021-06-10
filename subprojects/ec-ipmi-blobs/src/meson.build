ipmid_dep = declare_dependency(
  dependencies: dependency('libipmid'),
  compile_args: [
    '-DBOOST_ASIO_DISABLE_THREADS',
    '-DBOOST_ALL_NO_LIB',
    '-DBOOST_SYSTEM_NO_DEPRECATED',
    '-DBOOST_ERROR_CODE_HEADER_ONLY',
    '-DBOOST_COROUTINES_NO_DEPRECATION_WARNING',
  ])

# Function2 might not have a pkg-config. It is header only so just make
# sure we can access the needed symbols from the header.
function2_dep = dependency('function2', required: false)
meson.get_compiler('cpp').has_header_symbol(
  'function2/function2.hpp',
  'fu2::unique_function',
  dependencies: function2_dep,
  required: true)

ecblob_deps = [
  function2_dep,
  dependency('liburing'),
  dependency('stdplus'),
]

ecblob_lib = library(
  'ecblob',
  'cmd_net.cpp',
  'io_uring.cpp',
  implicit_include_directories: false,
  include_directories: ecblob_incs,
  dependencies: ecblob_deps,
  version: meson.project_version(),
  install: true)

ecblob_dep = declare_dependency(
  include_directories: ecblob_incs,
  link_with: ecblob_lib,
  dependencies: ecblob_deps)

ecblob_ipmid_deps = [
  ecblob_dep,
  ipmid_dep,
]

ecblob_ipmid_lib = library(
  'ecblobipmid',
  'ipmid.cpp',
  implicit_include_directories: false,
  dependencies: ecblob_ipmid_deps,
  version: meson.project_version(),
  install: true)

ecblob_ipmid_dep = declare_dependency(
  link_with: ecblob_ipmid_lib,
  dependencies: ecblob_ipmid_deps)

subdir('skm')
