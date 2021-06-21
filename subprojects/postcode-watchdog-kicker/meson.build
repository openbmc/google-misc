project(
  'poswatchdog',
  ['cpp', 'c'],
  version: '0.1',
  meson_version: '>=0.53.2',
  default_options: [
    'warning_level=3',
    'werror=true',
    'cpp_std=c++17',
  ])

headers = include_directories('.')

systemd = dependency('systemd')
systemunitdir = systemd.get_pkgconfig_variable('systemdsystemunitdir')

deps = [
  systemd,
  dependency('sdbusplus'),
  dependency('phosphor-dbus-interfaces'),
]

libexecdir = get_option('prefix') / get_option('libexecdir')

executable(
  'postwatchdogd',
  'config.cpp',
  'main.cpp',
  include_directories: headers,
  cpp_args: '-Wno-unused-parameter',
  dependencies: deps,
  install: true
)

configure_file(
  configuration: {'BIN': libexecdir / 'postwatchdogd'},
  input: 'postwd.service.in',
  output: 'postwd.service',
  install_mode: 'rw-r--r--',
  install_dir: systemunitdir)
