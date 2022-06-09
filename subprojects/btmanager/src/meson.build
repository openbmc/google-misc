boost_dep = dependency('boost', required : false)
thread_dep = dependency('threads')

nlohmann_json_dep = dependency('nlohmann_json', required : false)
if not nlohmann_json_dep.found()
    nlohmann_json_proj = subproject('nlohmann')
    nlohmann_json_dep = nlohmann_json_proj.get_variable('nlohmann_json_dep')
endif

deps = [
  thread_dep,
  boost_dep,
  nlohmann_json_dep,
  sdbusplus_dep,
  btdefinition_dep,
]

libexecdir = get_option('prefix') / get_option('libexecdir')

executable(
  'btmanager',
  'btmanager.cpp',
  'utils.cpp',
  'btStateMachine.cpp',
  'dbusHandler.cpp',
  generated_sources,
  include_directories: btmanager_incs,
  dependencies: deps,
  install: true,
  install_dir: libexecdir
)
