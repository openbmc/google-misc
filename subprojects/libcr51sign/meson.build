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

project('libcr51sign', 'c',
        version: '0.1', meson_version: '>=0.57.0',
        default_options: [
          'warning_level=2',
          'werror=false',
          'cpp_std=c++20'
        ])

# libssl part of openssl isn't used.
openssl = dependency('libcrypto')

libcr51sign_headers = include_directories('.')

libcr51sign_lib = library(
  'libcr51sign',
  'libcr51sign.c',
  'libcr51sign_support.c',
  include_directories: libcr51sign_headers,
  dependencies: openssl,
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
  'libcr51sign.h',
  'libcr51sign_support.h',
  subdir: 'libcr51sign')
