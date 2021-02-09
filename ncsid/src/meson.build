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
  dependency('stdplus', fallback: ['stdplus', 'stdplus']),
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
  install_mode: 'rwxr-xr-x',
  install_dir: get_option('libexecdir'))

install_data(
  'ncsid_lib.sh',
  install_mode: 'rw-r--r--',
  install_dir: get_option('libexecdir'))

install_data(
  'update_static_neighbors.sh',
  install_mode: 'rwxr-xr-x',
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
  'update-static-neighbors@.timer',
  install_mode: 'rw-r--r--',
  install_dir: systemunitdir)
