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

domain = 'com.google.gbmc.Hoth'

if_yaml_file = files('../Hoth.interface.yaml')

if_cpp = custom_target(
  'server.cpp',
  output: 'server.cpp',
  input: if_yaml_file,
  capture: true,
  command: [sdbuspp_prog, '-r', root, 'interface', 'server-cpp', domain])

if_hpp = custom_target(
  'server.hpp',
  output: 'server.hpp',
  input: if_yaml_file,
  capture: true,
  command: [sdbuspp_prog, '-r', root, 'interface', 'server-header', domain],
  install: true,
  install_dir: get_option('includedir') / 'com/google/gbmc/Hoth')

error_yaml_file = files('../Hoth.errors.yaml')

error_cpp = custom_target(
  'error.cpp',
  output: 'error.cpp',
  input: if_yaml_file,
  capture: true,
  command: [sdbuspp_prog, '-r', root, 'error', 'exception-cpp', domain])

error_hpp = custom_target(
  'error.hpp',
  output: 'error.hpp',
  input: if_yaml_file,
  capture: true,
  command: [sdbuspp_prog, '-r', root, 'error', 'exception-header', domain],
  install: true,
  install_dir: get_option('includedir') / 'com/google/gbmc/Hoth')
