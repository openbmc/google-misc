project(
  'espi_control',
  'cpp',
  version: '0.1',
  meson_version: '>=1.1.1',
  default_options: [
    'warning_level=3',
    'werror=true',
    'cpp_std=c++23',
  ],
)

executable(
  'npcm7xx-espi-control',
  'npcm7xx_espi_control.cpp',
  implicit_include_directories: false,
  dependencies:
  [
    dependency('stdplus'),
  ],
  install: true,
  install_dir: get_option('libexecdir'),
)
