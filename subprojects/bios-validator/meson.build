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

project('bios_validator', 'cpp',
        version: '0.1', meson_version: '>=0.53.0',
        default_options: [
          'warning_level=2',
          'werror=false',
          'cpp_std=c++17'
        ])

# The libcr51sign library
openssl = dependency('openssl')

libcr51sign_headers = include_directories('.')

libcr51sign_lib = library(
  'libcr51sign',
  ['libcr51sign.c', 'libcr51sign_support.c',],
  include_directories: libcr51sign_headers,
  dependencies: openssl,
  implicit_include_directories: false,
  version: meson.project_version(),
  install: true,
)

libcr51sign = declare_dependency(
  include_directories: libcr51sign_headers,
  link_with: libcr51sign_lib)

import('pkgconfig').generate(
  name: 'libcr51sign',
  description: 'CR51 signing verification utilities',
  version: meson.project_version(),
  libraries: libcr51sign)

install_headers('cr51_image_descriptor.h',
  'libcr51sign.h',
  'libcr51sign_support.h',
  subdir: 'libcr51sign')

# The BIOS validator tool
build_tests = get_option('tests')

trivial_fs = '''
#include <filesystem>

int main()
{
    std::filesystem::path("/");
    return 0;
}
'''

if not meson.get_compiler('cpp').links(trivial_fs)
  assert(
    meson.get_compiler('cpp').links(trivial_fs, args: '-lstdc++fs'),
    '<filesystem> not linkable')
  message('Using legacy stdc++fs')
  add_project_link_arguments('-lstdc++fs', language : 'cpp')
endif

bios_validator_headers = include_directories('.')

bios_validator_deps = [
  declare_dependency(compile_args: '-DFMT_STRING_ALIAS=1', dependencies: dependency('fmt')),
  dependency('libcr51sign'),
  dependency('phosphor-logging'),
  dependency('stdplus'),
]

libbiosvalidator = static_library(
  'bios_validator',
  'log_utils.cpp',
  'cli.cpp',
  include_directories: bios_validator_headers,
  implicit_include_directories: false,
  dependencies: bios_validator_deps,
)

bios_validator = declare_dependency(
  dependencies: bios_validator_deps,
  include_directories: bios_validator_headers,
  link_with: libbiosvalidator,
)

allow_prod_to_dev_downgrade = 'false'
if get_option('allow_prod_to_dev_downgrade')
  allow_prod_to_dev_downgrade = 'true'
endif

is_production_mode = 'true'
if not get_option('is_production_mode')
  is_production_mode = 'false'
endif

executable(
  'bios_validator',
  'main.cpp',
  include_directories: bios_validator_headers,
  implicit_include_directories: false,
  link_with: libbiosvalidator,
  install: true,
  cpp_args: ['-DALLOW_PROD_TO_DEV_DOWNGRADE=' + allow_prod_to_dev_downgrade,
    '-DIS_PRODUCTION_MODE=' + is_production_mode]
)

if not build_tests.disabled()
  subdir('test')
endif
