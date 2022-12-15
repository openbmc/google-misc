project(
    'phosphor-metric-collector',
    'cpp',
    version: '1.0',
    default_options: [
        'cpp_std=c++20',
    ],
    meson_version: '>=0.63.0',
)

cpp = meson.get_compiler('cpp')

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

executable(
    'metric-collector',
    'metricCollector.cpp',
    'daemon.cpp',
    'port.cpp',
    'utils.cpp',
    dependencies: [
        dependency('phosphor-dbus-interfaces'),
        dependency('phosphor-logging'),
        dependency('sdbusplus'),
        dependency('sdeventplus'),
        fmt_dep,
    ],
    install: true,
    install_dir: get_option('libexecdir')
)

systemd = dependency('systemd')
configure_file(
  input: 'phosphor-metric-collector.service.in',
  output: 'phosphor-metric-collector.service',
  configuration: {'LIBEXECDIR': get_option('prefix') / get_option('libexecdir')},
  install: true,
  install_dir: systemd.get_pkgconfig_variable('systemdsystemunitdir'))
