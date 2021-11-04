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

ncsid_headers = include_directories('.')

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

ncsid_deps = [
  fmt_dep,
  dependency('sdbusplus', fallback: ['sdbusplus', 'sdbusplus_dep']),
  dependency('stdplus', fallback: ['stdplus', 'stdplus_dep']),
]

ncsid_lib = static_library(
  'ncsid',
  [
    'net_config.cpp',
    'net_iface.cpp',
    'net_sockio.cpp',
    'ncsi_sockio.cpp',
    'ncsi_state_machine.cpp',
    'platforms/nemora/portable/ncsi_fsm.c',
    'platforms/nemora/portable/ncsi_client.c',
    'platforms/nemora/portable/ncsi_server.c',
  ],
  include_directories: ncsid_headers,
  implicit_include_directories: false,
  dependencies: ncsid_deps)

ncsid = declare_dependency(
  dependencies: ncsid_deps,
  include_directories: ncsid_headers,
  link_with: ncsid_lib)

executable(
  'ncsid',
  'ncsid.cpp',
  implicit_include_directories: false,
  dependencies: ncsid,
  install: true,
  install_dir: get_option('libexecdir'))

normalize_ip = executable(
  'normalize_ip',
  'normalize_ip.c',
  implicit_include_directories: false,
  install: true)

normalize_mac = executable(
  'normalize_mac',
  'normalize_mac.c',
  implicit_include_directories: false,
  install: true)

install_data(
  'ncsid_udhcpc4.script',
  'ncsid_udhcpc6.script',
  'update_ra_gw.sh',
  'update_ra_neighbor.sh',
  'update_static_neighbors.sh',
  install_mode: 'rwxr-xr-x',
  install_dir: get_option('libexecdir'))

install_data(
  'ncsid_lib.sh',
  install_mode: 'rw-r--r--',
  install_dir: get_option('libexecdir'))

systemd = dependency('systemd')
systemunitdir = systemd.get_pkgconfig_variable('systemdsystemunitdir')

libexecdir = get_option('prefix') / get_option('libexecdir')

configure_file(
  configuration: {'BIN': libexecdir / 'ncsid'},
  input: 'ncsid@.service.in',
  output: 'ncsid@.service',
  install_mode: 'rw-r--r--',
  install_dir: systemunitdir)

configure_file(
  configuration: {'BIN': libexecdir / 'update_ra_gw.sh'},
  input: 'update-ra-gw@.service.in',
  output: 'update-ra-gw@.service',
  install_mode: 'rw-r--r--',
  install_dir: systemunitdir)

configure_file(
  configuration: {'BIN': libexecdir / 'update_ra_neighbor.sh'},
  input: 'update-ra-neighbor@.service.in',
  output: 'update-ra-neighbor@.service',
  install_mode: 'rw-r--r--',
  install_dir: systemunitdir)

configure_file(
  configuration: {'BIN': libexecdir / 'update_static_neighbors.sh'},
  input: 'update-static-neighbors@.service.in',
  output: 'update-static-neighbors@.service',
  install_mode: 'rw-r--r--',
  install_dir: systemunitdir)

configure_file(
  configuration: {
    'SCRIPT': libexecdir / 'ncsid_udhcpc4.script'},
  input: 'dhcp4@.service.in',
  output: 'dhcp4@.service',
  install_mode: 'rw-r--r--',
  install_dir: systemunitdir)

configure_file(
  configuration: {
    'SCRIPT': libexecdir / 'ncsid_udhcpc6.script'},
  input: 'dhcp6@.service.in',
  output: 'dhcp6@.service',
  install_mode: 'rw-r--r--',
  install_dir: systemunitdir)

install_data(
  'nic-hostful@.target',
  'nic-hostless@.target',
  'update-ra-neighbor@.timer',
  'update-static-neighbors@.timer',
  install_mode: 'rw-r--r--',
  install_dir: systemunitdir)
