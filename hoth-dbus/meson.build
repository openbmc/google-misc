# Copyright 2021 Google LLC
#
# Licensed under the Apache License, Version 2.0 (the "License");
# you may not use this file except in compliance with the License.
# You may obtain a copy of the License at
#
#      http://www.apache.org/licenses/LICENSE-2.0
#
# Unless required by applicable law or agreed to in writing, software
# distributed under the License is distributed on an "AS IS" BASIS,
# WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
# See the License for the specific language governing permissions and
# limitations under the License.

project(
  'host-dbus',
  ['cpp'],
  version: '0.1',
  meson_version: '>=0.53.2',
  default_options: [
    'warning_level=3',
    'werror=true',
    'cpp_std=c++17',
  ])

root = meson.current_source_dir()
inc = include_directories('.')

sdbusplus_dep = dependency('sdbusplus', required: false)
if sdbusplus_dep.found()
  sdbuspp_prog = find_program('sdbus++')
else
  sdbusplus_proj = subproject('sdbusplus', required: true)
  sdbusplus_dep = sdbusplus_proj.get_variable('sdbusplus_dep')
  sdbuspp_prog = sdbusplus_proj.get_variable('sdbusplusplus_prog')
endif

subdir('com/google/gbmc/Hoth')

deps = [
  dependency('phosphor-dbus-interfaces'),
  sdbusplus_dep,
]

lib = library(
  'hoth-dbus',
  if_cpp,
  if_hpp,
  error_cpp,
  error_hpp,
  implicit_include_directories: false,
  include_directories: inc,
  version: meson.project_version(),
  dependencies: deps,
  install: true)

dep = declare_dependency(
  dependencies: deps,
  include_directories: inc,
  sources: [if_hpp, error_hpp],
  link_with: lib)

reqs = []
foreach d : deps
  if d.type_name() == 'pkgconfig'
    reqs += d
  endif
endforeach

import('pkgconfig').generate(
  name: 'hoth-dbus',
  description: 'Hoth DBus Bindings',
  version: meson.project_version(),
  libraries: lib,
  requires: reqs)
