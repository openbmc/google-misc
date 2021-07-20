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

flasher_deps = [
  dependency('stdplus', fallback: ['stdplus', 'stdplus_dep']),
]

flasher_pre = declare_dependency(
  include_directories: flasher_includes,
  dependencies: flasher_deps)

flasher_lib = library(
  'flasher',
  'args.cpp',
  'convert.cpp',
  'device.cpp',
  'device/fake.cpp',
  'device/mtd.cpp',
  'file.cpp',
  'file/memory.cpp',
  'file/simple.cpp',
  'logging.cpp',
  'mod.cpp',
  'mutate.cpp',
  'mutate/rot128.cpp',
  'ops.cpp',
  implicit_include_directories: false,
  include_directories: flasher_internal_includes,
  dependencies: flasher_pre,
  version: meson.project_version(),
  install: true)

flasher_dep = declare_dependency(
  dependencies: flasher_pre,
  link_with: flasher_lib)

flasher_reqs = []
foreach dep : flasher_deps
  if dep.type_name() == 'pkgconfig'
    flasher_reqs += dep
  endif
endforeach

import('pkgconfig').generate(
  flasher_lib,
  description: 'Common flash tool library',
  version: meson.project_version(),
  requires: flasher_reqs)

executable(
  'flasher',
  'main.cpp',
  implicit_include_directories: false,
  include_directories: flasher_internal_includes,
  dependencies: flasher_dep,
  install: true)
