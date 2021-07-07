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
  'google-misc',
  'c',
  version: '0.1',
  meson_version: '>=0.53.2',
  default_options: [
    'warning_level=3',
    'werror=true',
    'cpp_std=c++17',
    'c_std=c18',
    'tests=' + (meson.is_subproject() ? 'disabled' : 'auto'),
  ])

# Dummy file to make code coverage happy
executable('hello', 'hello.c')

# Meson has no feature option to str...
tests_str = get_option('tests').enabled() ? 'enabled' : 'disabled'
if get_option('tests').auto()
  tests_str = 'auto'
endif

subproject('acpi-power-state-daemon')
subproject('ncsid', default_options: 'tests=' + tests_str)
subproject('metrics-ipmi-blobs', default_options: 'tests=' + tests_str)
subproject('nemora-postd')
