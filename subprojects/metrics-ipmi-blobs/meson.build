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
  ['cpp', 'c'],
  version: '0.1',
  default_options: [
    'warning_level=3',
    'werror=true',
    'cpp_std=c++23',
    'c_std=c18',
    'tests=' + (meson.is_subproject() ? 'disabled' : 'auto'),
  ],
)

nanopb = find_program('nanopb_generator.py', native: true, required: false)
if not nanopb.found()
  nanopb_opts = import('cmake').subproject_options()
  nanopb_opts.add_cmake_defines({'BUILD_SHARED_LIBS': 'ON'})
  nanopb_proj = import('cmake').subproject('nanopb', options: nanopb_opts)
  nanopb = find_program(meson.global_source_root() + '/subprojects/nanopb/generator/nanopb_generator.py', native: true)
  nanopb_dep = nanopb_proj.dependency('protobuf_nanopb')
else
  nanopb_dep = meson.get_compiler('cpp').find_library('protobuf-nanopb')
endif

nanopb_kwargs = {
  'output': [
    '@BASENAME@.pb.n.h',
    '@BASENAME@.pb.n.c',
  ],
  'command': [
    nanopb,
    '-q',
    '-s', 'packed_struct:0',
    '-H.n.h',
    '-S.n.c',
    '-I' + import('fs').relative_to(meson.current_source_dir(), meson.global_build_root()),
    '-D' + import('fs').relative_to(meson.current_build_dir(), meson.global_build_root()),
    '@INPUT@',
  ],
}

tgt = custom_target(
  'metricblob.pb.n.hc',
  input: 'metricblob.proto',
  kwargs: nanopb_kwargs)
metrics_nanopb_hdr = tgt[0]
metrics_nanopb_src = tgt[1]

metrics_nanopb_pre = declare_dependency(
  include_directories: include_directories('.'),
  sources: metrics_nanopb_hdr,
  dependencies: [
    nanopb_dep,
  ])

metrics_nanopb_lib = static_library(
  'metrics_nanopb',
  metrics_nanopb_src,
  implicit_include_directories: false,
  dependencies: metrics_nanopb_pre)

metrics_nanopb_dep = declare_dependency(
  dependencies: metrics_nanopb_pre,
  link_with: metrics_nanopb_lib)

pre = declare_dependency(
  include_directories: include_directories('.'),
  dependencies: [
    metrics_nanopb_dep,
    dependency('phosphor-logging'),
    dependency('phosphor-ipmi-blobs'),
    dependency('sdbusplus'),
  ])

lib = static_library(
  'metricsblob',
  'util.cpp',
  'handler.cpp',
  'metric.cpp',
  implicit_include_directories: false,
  dependencies: pre)

dep = declare_dependency(
  dependencies: pre,
  link_with: lib)

shared_module(
  'metricsblob',
  'main.cpp',
  dependencies: dep,
  implicit_include_directories: false,
  install: true,
  install_dir: get_option('libdir') / 'blob-ipmid')

if get_option('tests').allowed()
  subdir('test')
endif
