systemd = dependency('systemd')
systemunitdir = systemd.get_pkgconfig_variable('systemdsystemunitdir')

configure_file(
  configuration: {'BIN': libexecdir / 'btmanager'},
  input: 'btmanager.service.in',
  output: 'btmanager.service',
  install_mode: 'rw-r--r--',
  install_dir: systemunitdir
)
