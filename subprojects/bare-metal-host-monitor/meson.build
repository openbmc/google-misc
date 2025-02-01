project(
    'host_gpio_monitor',
    'cpp',
    version: '0.1',
    meson_version: '>=1.1.1',
    default_options: ['warning_level=3', 'werror=true', 'cpp_std=c++23'],
)

absls = ['flags', 'flags_internal', 'flags_parse']
libabsl_deps = []
foreach absl : absls
    libabsl_deps += dependency('absl_' + absl)
endforeach

executable(
    'host_gpio_monitor',
    'host_gpio_monitor.cpp',
    implicit_include_directories: false,
    dependencies: [
        dependency('stdplus'),
        dependency('phosphor-logging'),
        libabsl_deps,
    ],
    install: true,
    install_dir: get_option('libexecdir'),
)

systemd = dependency('systemd')
systemunitdir = systemd.get_variable('systemdsystemunitdir')

libexecdir = get_option('prefix') / get_option('libexecdir')

configure_file(
    configuration: {'BIN': libexecdir / 'host_gpio_monitor'},
    input: 'host-gpio-monitor@.service.in',
    output: 'host-gpio-monitor@.service',
    install_mode: 'rw-r--r--',
    install_dir: systemunitdir,
)
