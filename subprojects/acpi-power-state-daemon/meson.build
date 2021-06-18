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
  'acpi_power_stated',
  'cpp',
  version: '0.1',
  default_options: [
    'warning_level=3',
    'werror=true',
    'cpp_std=c++17',
  ],
)

headers = include_directories('.')

systemd = dependency('systemd')
systemunitdir = systemd.get_pkgconfig_variable('systemdsystemunitdir')

deps = [
  systemd,
  dependency('sdbusplus', fallback: ['sdbusplus', 'sdbusplus_dep']),
  dependency(
    'phosphor-dbus-interfaces',
    fallback: ['phosphor-dbus-interfaces', 'phosphor_dbus_interfaces_dep']),
]

bindir = get_option('prefix') / get_option('bindir')

executable(
  'acpi_power_stated',
  'acpi_power_state.cpp',
  include_directories: headers,
  implicit_include_directories: false,
  dependencies: deps,
  install: true,
  install_dir: bindir)

configure_file(
  configuration: {'BIN': bindir / 'acpi_power_stated'},
  input: 'acpi-power-state.service.in',
  output: 'acpi-power-state.service',
  install_mode: 'rw-r--r--',
  install_dir: systemunitdir)

install_data(
  'host-s0-state.target',
  'host-s5-state.target',
  install_mode: 'rw-r--r--',
  install_dir: systemunitdir)
