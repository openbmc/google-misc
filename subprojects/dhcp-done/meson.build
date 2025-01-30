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
  'dhcp-done',
  'cpp',
  version: '0.1',
  meson_version: '>=1.1.1',
  default_options: [
    'warning_level=3',
    'werror=true',
    'cpp_std=c++23',
  ],
)

deps = [
  dependency('sdeventplus'),
  dependency('stdplus'),
]

libexecdir = get_option('prefix') / get_option('libexecdir')
bindir = get_option('prefix') / get_option('bindir')

fileio_lib = static_library(
  'fileio',
  [
    'file-io.cpp',
  ],
  implicit_include_directories: false)

executable(
  'dhcp-done',
  'dhcp-done.cpp',
  implicit_include_directories: false,
  dependencies: deps,
  link_with : fileio_lib,
  install: true,
  install_dir: libexecdir)

executable(
  'update-dhcp-status',
  'update-dhcp-status.cpp',
  implicit_include_directories: false,
  dependencies: deps,
  link_with : fileio_lib,
  install: true,
  install_dir: bindir)

systemd = dependency('systemd')
systemunitdir = systemd.get_variable('systemdsystemunitdir')

configure_file(
  configuration: {'BIN': libexecdir / 'dhcp-done'},
  input: 'dhcp-done.service.in',
  output: 'dhcp-done.service',
  install_mode: 'rw-r--r--',
  install_dir: systemunitdir)

build_tests = get_option('tests')

#if build_tests.allowed()
subdir('test')
#endif
