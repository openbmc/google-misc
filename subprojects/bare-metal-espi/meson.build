project(
  'bare_metal_espi',
  'cpp',
  version: '0.1',
  meson_version: '>=0.58.0',
  default_options: [
    'warning_level=3',
    'werror=true',
    'cpp_std=c++20',
  ],
)

executable(
  'npcm7xx-espi-control',
  'npcm7xx_espi_control.cpp',
  implicit_include_directories: false,
  dependencies:
  [
    dependency('fmt'),
    dependency('stdplus'),
  ],
  install: true,
  install_dir: get_option('libexecdir'),
)

systemd_dep = dependency('systemd')
conf_data = configuration_data()
conf_data.set('libexecdir', get_option('prefix') / get_option('libexecdir'))

configure_file(
  configuration: conf_data,
  input: 'gbmc-disable-espi.service.in',
  output: 'gbmc-disable-espi.service',
  install_dir: systemd_dep.get_variable('systemdsystemunitdir'),
  install: true,
)
