boost_dep = dependency('boost', required : false)
thread_dep = dependency('threads')

nlohmann_json_dep = dependency('nlohmann_json', required : false)
if not nlohmann_json_dep.found()
    nlohmann_json_proj = subproject('nlohmann')
    nlohmann_json_dep = nlohmann_json_proj.get_variable('nlohmann_json_dep')
endif

sdbusplus_dep = dependency('sdbusplus', required : false)
if not sdbusplus_dep.found()
    sdbusplus_proj = subproject('sdbusplus')
    sdbusplus_dep = sdbusplus_proj.get_variable('sdbusplus_dep')
endif

repo_root = run_command(
    'git',
    'rev-parse',
    '--show-toplevel'
).stdout().strip()
sdbusgendir = repo_root / 'subprojects/sdbusplus/tools/sdbus++-gendir'
sdbusgen_prog = find_program(sdbusgendir)
message(sdbusgendir)

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
  thread_dep,
  boost_dep,
  nlohmann_json_dep,
  sdbusplus_dep,
]

libexecdir = get_option('prefix') / get_option('libexecdir')

executable(
  'btmanager',
  'btmanager.cpp',
  'utils.cpp',
  'btStateMachine.cpp',
  'btDefinitions.cpp',
  'dbusHandler.cpp',
  btmanager_files,
  include_directories : btmanager_incs,
  dependencies: deps,
  install: true,
  install_dir: libexecdir
)
