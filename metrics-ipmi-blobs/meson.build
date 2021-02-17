project(
  'metrics-ipmi-blobs',
  'cpp',
  version: '0.1',
  default_options: [
    'warning_level=3',
    'werror=true',
    'cpp_std=c++17',
    'tests=' + (meson.is_subproject() ? 'disabled' : 'auto'),
  ],
)

headers = include_directories('.')

deps = [
  dependency('phosphor-logging'),
  dependency('phosphor-ipmi-blobs'),
  dependency('protobuf'),
]

lib = static_library(
  'metricsblob',
  'util.cpp',
  'handler.cpp',
  'metric.cpp',
  custom_target(
    'metricblob_proto',
    command: [
      find_program('protoc', native: true),
      '--proto_path=@CURRENT_SOURCE_DIR@',
      '--cpp_out=@OUTDIR@',
      '@INPUT@'
    ],
    output: [
      'metricblob.pb.cc',
      'metricblob.pb.h',
    ],
    input: 'metricblob.proto'),
  include_directories: headers,
  implicit_include_directories: false,
  dependencies: deps)

dep = declare_dependency(
  dependencies: deps,
  include_directories: headers,
  link_with: lib)

shared_module(
  'metricsblob',
  'main.cpp',
  dependencies: dep,
  implicit_include_directories: false,
  install: true,
  install_dir: get_option('libdir') / 'ipmid-providers')

if not get_option('tests').disabled()
  subdir('test')
endif
