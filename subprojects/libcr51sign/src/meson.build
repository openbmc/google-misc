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

libcr51sign_deps = [
  dependency('libcrypto'),
]

libcr51sign_cflags = [
  '-Wno-pedantic',
  '-Wno-implicit-fallthrough'
]

libcr51sign_pre = declare_dependency(
  include_directories: libcr51sign_includes,
  compile_args: libcr51sign_cflags,
  dependencies: libcr51sign_deps)

libcr51sign_lib = library(
  'cr51sign',
  'libcr51sign.c',
  'libcr51sign_support.c',
  'libcr51sign_mauv.c',
  dependencies: libcr51sign_pre,
  c_args: [
    # Temporarily ignore until updates land
    '-Wno-error=deprecated-declarations',
  ],
  implicit_include_directories: false,
  version: meson.project_version(),
  install: true)

libcr51sign_dep = declare_dependency(
  link_with: libcr51sign_lib,
  dependencies: libcr51sign_pre)

libcr51sign_reqs = []
foreach dep : libcr51sign_deps
  if dep.type_name() == 'pkgconfig'
    libcr51sign_reqs += dep
  endif
endforeach

import('pkgconfig').generate(
  libcr51sign_lib,
  name: 'libcr51sign',
  description: 'CR51 signing verification utilities',
  requires: libcr51sign_reqs,
  extra_cflags: libcr51sign_cflags,
  version: meson.project_version())
