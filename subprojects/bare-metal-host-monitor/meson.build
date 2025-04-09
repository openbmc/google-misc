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
    include_directories: include_directories('.'),
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

# Set up config file with the dbus information that needs to be monitored.
conf_data = configuration_data()
conf_data.set_quoted(
    'DBUS_SERVICE_NAME',
    get_option('host_monitor_service_name'),
)
conf_data.set_quoted('DBUS_OBJECT_PATH', get_option('host_monitor_object_path'))
conf_data.set_quoted('DBUS_INTERFACE', get_option('host_monitor_interface'))
conf_data.set_quoted('DBUS_PROPERTY_NAME', get_option('host_monitor_property'))
conf_data.set_quoted(
    'DBUS_PROPERTY_HOST_IN_RESET_VALUE',
    get_option('host_monitor_host_in_reset_value'),
)
conf_data.set_quoted(
    'DBUS_PROPERTY_HOST_RUNNING_VALUE',
    get_option('host_monitor_host_running_value'),
)
configure_file(output: 'host_gpio_monitor_conf.hpp', configuration: conf_data)
