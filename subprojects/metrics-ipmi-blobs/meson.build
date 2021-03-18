# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

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
