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
  default_options: [
    'warning_level=3',
    'werror=true',
    'cpp_std=c++20',
  ],
)

deps = [
  dependency('sdeventplus'),
  dependency('stdplus'),
]

libexecdir = get_option('prefix') / get_option('libexecdir')

executable(
  'dhcp-done',
  'dhcp-done.cpp',
  implicit_include_directories: false,
  dependencies: deps,
  install: true,
  install_dir: libexecdir)

systemd = dependency('systemd')
systemunitdir = systemd.get_pkgconfig_variable('systemdsystemunitdir')

configure_file(
  configuration: {'BIN': libexecdir / 'dhcp-done'},
  input: 'dhcp-done.service.in',
  output: 'dhcp-done.service',
  install_mode: 'rw-r--r--',
  install_dir: systemunitdir)
