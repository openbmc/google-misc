project(
  'hoth-ipmi-blobs',
  'cpp',
  version: '0.1',
  meson_version: '>=0.51.0',
  default_options: [
    'b_lundef=false',
    'cpp_std=c++17',
    'warning_level=3',
    'werror=true',
  ])

sdbusplus_dep = dependency('sdbusplus', fallback: ['sdbusplus','sdbusplus_dep'])
stdplus_dep = dependency('stdplus', fallback: ['stdplus','stdplus_dep'])
ipmi_blob_dep = dependency('phosphor-ipmi-blobs')
phosphor_logging_dep = dependency('phosphor-logging')

meson.get_compiler('cpp').has_header_symbol(
  'ipmid/api.h',
  'ipmid_get_sd_bus_connection')

hothblob_pre = declare_dependency(
  include_directories: include_directories('.'),
  dependencies: [
    ipmi_blob_dep,
    sdbusplus_dep,
    stdplus_dep,
  ])

hothblob_lib = library(
  'hothblob',
  'dbus.cpp',
  'hoth.cpp',
  implicit_include_directories: false,
  dependencies: hothblob_pre,
  version: meson.project_version(),
  install: true)

hothblob_dep = declare_dependency(
  link_with: hothblob_lib,
  dependencies: hothblob_pre)

if not get_option('tests').disabled()
  subdir('test')
endif

subdir('command')
