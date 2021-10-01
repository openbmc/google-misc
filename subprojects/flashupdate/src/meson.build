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

# if not get_option('dev').disabled()
#  add_project_arguments('-DDEV_WORKFLOW', language: 'cpp')
# endif

stdplus_dep = dependency('stdplus', fallback: ['stdplus', 'stdplus_dep'])


flasher_dep = dependency('flasher', required: true)

json_dep = dependency('', required: false)
has_json = meson.get_compiler('cpp').has_header_symbol(
  'nlohmann/json.hpp',
  'nlohmann::json::string_t',
  required: false)
if not has_json
  json_dep = dependency(
    'nlohmann_json',
    fallback: ['nlohmann_json', 'nlohmann_json_dep'],
    disabler: true,
    required: false)
endif

libcr51sign_dep = dependency('libcr51sign')

libcr51_deps = [
  fmt_dep,
  libcr51sign_dep,
]

libcr51sign_dep = dependency('libcr51sign', fallback : ['libcr51sign', 'libcr51sign'])

libflashupdate_deps = [
  flasher_dep,
  json_dep,
  fmt_dep,
  libcr51sign_dep,
]

libflashupdate_lib = library(
  'flashupdate',
  'args.cpp',
  'config.cpp',
  'info.cpp',
  'ops.cpp',
  'logging.cpp',
  version: meson.project_version(),
  dependencies: libflashupdate_deps,
  include_directories: [flashupdate_includes, flashupdate_internal_includes],
  install: true)

flashupdate_reqs = []
foreach dep : libflashupdate_deps
  if dep.type_name() == 'pkgconfig'
    flashupdate_reqs += dep
  endif
endforeach

import('pkgconfig').generate(
  libflashupdate_lib,
  description: 'Common flashupdate library',
  version: meson.project_version(),
  requires: flashupdate_reqs)

libflashupdate = declare_dependency(
  dependencies: libflashupdate_deps,
  include_directories: [flashupdate_includes, flashupdate_internal_includes],
  link_with: libflashupdate_lib)

executable(
  'flashupdate',
  'flashupdate.cpp',
  implicit_include_directories: false,
  dependencies: libflashupdate,
  include_directories: [flashupdate_includes, flashupdate_internal_includes],
  install: true,
  install_dir: get_option('bindir'))

