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

project('libcr51sign', ['c', 'cpp'],
        version: '0.1', meson_version: '>=0.57.0',
        default_options: [
          'warning_level=2',
          'werror=false',
          'cpp_std=c++20'
        ])

if get_option('prod-to-dev-downgrade')
  add_project_arguments('-DALLOW_PROD_TO_DEV_DOWNGRADE', language: 'cpp')
endif

if get_option('prod-to-dev-downgrade')
  add_project_arguments('-DIS_PRODUCTION_MODE', language: 'cpp')
endif

# libssl part of openssl isn't used.
openssl = dependency('libcrypto')

fmt_dep = dependency('fmt', required: false)
if not fmt_dep.found()
  fmt_opt = import('cmake').subproject_options()
  fmt_opt.append_compile_args(
    'c++',
    ['-DCMAKE_POSITION_INDEPENDENT_CODE=ON',
      '-DMASTER_PROJECT=OFF'])
  fmt_proj = cmake.subproject('fmt', options: fmt_opt, required: false)
  assert(fmt_proj.found(), 'fmtlib is required')
  fmt_dep = fmt_proj.dependency('fmt')
endif

libcr51sign_deps = [openssl, fmt_dep]
libcr51sign_headers = include_directories('.')

libcr51sign_lib = library(
  'libcr51sign',
  'libcr51sign.c',
  'libcr51sign_support.c',
  'cr51sign_validator.cpp',
  include_directories: libcr51sign_headers,
  dependencies: libcr51sign_deps,
  implicit_include_directories: false,
  version: meson.project_version(),
  install: true)

pkg = import('pkgconfig')
pkg.generate(
  libcr51sign_lib,
  name: 'libcr51sign',
  description: 'CR51 signing verification utilities',
  requires: openssl,
  version: meson.project_version())

install_headers(
  'cr51_image_descriptor.h',
  'cr51sign_validator.h',
  'libcr51sign.h',
  'libcr51sign_support.h',
  subdir: 'libcr51sign')
