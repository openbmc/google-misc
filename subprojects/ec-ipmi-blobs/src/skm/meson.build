ecskmblob_deps = [
  ecblob_dep,
  dependency('fmt'),
  function2_dep,
  ipmid_dep,
  dependency('phosphor-ipmi-blobs'),
]

ecskmblob = static_library(
  'ecskmblob',
  'handler.cpp',
  'cmd.cpp',
  implicit_include_directories: false,
  include_directories: ecblob_incs,
  dependencies: ecskmblob_deps)

ecskmblob_dep = declare_dependency(
  include_directories: ecblob_incs,
  link_with: ecskmblob,
  dependencies: ecskmblob_deps)

shared_library(
  'ecskmblob',
  'main.cpp',
  implicit_include_directories: false,
  dependencies: [
    ecskmblob_dep,
    ecblob_ipmid_dep,
  ],
  version: '0',
  install: true,
  install_dir: get_option('libdir') / 'blob-ipmid')
