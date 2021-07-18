# Function2 might not have a pkg-config. It is header only so just make
# sure we can access the needed symbols from the header.
function2_dep = dependency('function2', required: false)
has_function2 = meson.get_compiler('cpp').has_header_symbol(
  'function2/function2.hpp',
  'fu2::unique_function',
  dependencies: function2_dep)

hothcommand_pre = declare_dependency(
  include_directories: include_directories('.'),
  dependencies: [
    function2_dep,
    hothblob_dep,
    phosphor_logging_dep,
    sdbusplus_dep,
    stdplus_dep,
  ])

hothcommand_lib = static_library(
  'hothcommand',
  'dbus_command.cpp',
  'hoth_command.cpp',
  implicit_include_directories: false,
  dependencies: hothcommand_pre)

hothcommand_dep = declare_dependency(
  link_with: hothcommand_lib,
  dependencies: hothcommand_pre)

shared_module(
  'hothcommand',
  'main_command.cpp',
  implicit_include_directories: false,
  dependencies: hothcommand_dep,
  install: true,
  install_dir: get_option('libdir') / 'blob-ipmid')

if not get_option('tests').disabled()
  subdir('test')
endif
