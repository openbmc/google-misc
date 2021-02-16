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

headers = include_directories('.')

fmt_dep = dependency('fmt', required: false)
if not fmt_dep.found()
  fmt_proj = import('cmake').subproject(
    'fmt',
    cmake_options: [
      '-DCMAKE_POSITION_INDEPENDENT_CODE=ON',
      '-DMASTER_PROJECT=OFF'
    ],
    required: false)
  assert(fmt_proj.found(), 'fmtlib is required')
  fmt_dep = fmt_proj.dependency('fmt')
endif

deps = [
  fmt_dep,
  dependency('stdplus', fallback: ['stdplus', 'stdplus']),
  dependency('sdbusplus', fallback: ['sdbusplus', 'sdbusplus_dep']),
  dependency('sdeventplus', fallback: ['sdeventplus', 'sdeventplus']),
  dependency('libsystemd'),
]

lib = static_library(
  'kcsbridged',
  'args.cpp',
  include_directories: headers,
  implicit_include_directories: false,
  dependencies: deps)

dep = declare_dependency(
  dependencies: deps,
  include_directories: headers,
  link_with: lib)

libexecdir = get_option('prefix') / get_option('libexecdir')

executable(
  'kcsbridged',
  'main.cpp',
  implicit_include_directories: false,
  dependencies: dep,
  install: true,
  install_dir: libexecdir)

systemd = dependency('systemd', required: false)
if systemd.found()
  configure_file(
    configuration: {'BIN': libexecdir / 'kcsbridged'},
    input: 'kcsbridge@.service.in',
    output: 'kcsbridge@.service',
    install_mode: 'rw-r--r--',
    install_dir: systemd.get_pkgconfig_variable('systemdsystemunitdir'))
endif
