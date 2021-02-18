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
    'cpp_std=c++17',
  ],
)

add_project_arguments(
  '-Wno-unused-parameter',
  language:'cpp')

protoc = find_program('protoc', required: true)

gen = generator(protoc,
  output: [
    '@BASENAME@.pb.cc', '@BASENAME@.pb.h'
  ],
  arguments : ['--proto_path=@CURRENT_SOURCE_DIR@', '--cpp_out=@BUILD_DIR@', '@INPUT@'])
generated = gen.process(['metricblob.proto'])

shared_library(
  'metrics',
  'main.cpp',
  'handler.cpp',
  'metric.cpp',
  'util.cpp',
  generated,
  install: true,
  install_dir: '/usr/lib/blob-ipmid/',
  dependencies: [
    dependency('phosphor-logging'),
    dependency('phosphor-ipmi-blobs'),
    dependency('protobuf'),
  ],
  version: '0',
)

gtest_dep = dependency('gtest')
text_executable = executable('testprog', [
    'test/util_test.cpp', 'util.cpp'],
    dependencies: [ gtest_dep, dependency('phosphor-logging') ])
test('gtest test', text_executable)
