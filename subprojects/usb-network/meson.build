# Copyright 2022 Google LLC
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
  'google-usb-network',
  'cpp',
  version: '0.1',
  meson_version: '>=0.57.0',
  default_options: [
    'warning_level=3',
    'cpp_std=c++20',
  ])

fmt_dep = dependency('fmt', required: false)
if not fmt_dep.found()
  fmt_opts = import('cmake').subproject_options()
  fmt_opts.add_cmake_defines({
    'CMAKE_POSITION_INDEPENDENT_CODE': 'ON',
    'MASTER_PROJECT': 'OFF',
  })
  fmt_proj = import('cmake').subproject(
    'fmt',
    options: fmt_opts,
    required: false)
  assert(fmt_proj.found(), 'fmtlib is required')
  fmt_dep = fmt_proj.dependency('fmt')
endif

kcsbridged = executable(
  'google-usb-dynamic',
  'google-usb-dynamic.cpp',
  implicit_include_directories: false,
  dependencies: [
    fmt_dep,
    dependency('libsystemd'),
    dependency('sdbusplus'),
    dependency('sdeventplus'),
    dependency('stdplus'),
  ],
  install: true,
  install_dir: get_option('libexecdir'))

