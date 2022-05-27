sdbusplus_dep = dependency('sdbusplus', required : false)
if not sdbusplus_dep.found()
    sdbusplus_proj = subproject('sdbusplus')
    sdbusplus_dep = sdbusplus_proj.get_variable('sdbusplus_dep')
endif
sdbusgen_prog = find_program('sdbus++-gendir')
sdbusplusplus_prog = find_program('sdbus++', native: true)
sdbuspp_gen_meson_prog = find_program('sdbus++-gen-meson', native: true)

btmanager_buildroot = meson.current_build_dir()
btmanager_files = files(
    run_command(
        sdbusgen_prog,
        '--tool', sdbusplusplus_prog,
        '--output', btmanager_buildroot,
        'xyz/openbmc_project/Time',
        check: true
    ).stdout().strip().split('\n')
)

deps = [
  sdbusplus_dep,
]

libexecdir = get_option('prefix') / get_option('libexecdir')

executable(
  'btmanager',
  'btmanager.cpp',
  btmanager_files,
  dependencies: deps,
  install: true,
  install_dir: libexecdir
)
