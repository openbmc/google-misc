systemd = dependency('systemd')
systemunitdir = systemd.get_pkgconfig_variable('systemdsystemunitdir')

configure_file(
  configuration: {'BIN': libexecdir / 'btmanager'},
  input: 'btmanager.service.in',
  output: 'btmanager.service',
  install_mode: 'rw-r--r--',
  install_dir: systemunitdir
)

configure_file(
  copy: true,
  input: 'record-down-end.sh',
  output: 'record-down-end.sh',
  install_mode: 'rwxr-xr-x',
  install_dir: get_option('libexecdir')
)

configure_file(
  configuration: {'BIN': libexecdir / 'record-down-end.sh'},
  input: 'record-down-end.service.in',
  output: 'record-down-end.service',
  install_mode: 'rw-r--r--',
  install_dir: systemunitdir
)
