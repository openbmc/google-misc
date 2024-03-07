# Copyright 2024 Google LLC
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
#
gtest = dependency('gtest', main: true, disabler: true, required: false)
if not gtest.found()
  gtest_proj = import('cmake').subproject(
    'googletest',
    cmake_options: [
      '-DCMAKE_CXX_FLAGS=-Wno-pedantic',
    ],
    required: false)
  if gtest_proj.found()
    gtest = declare_dependency(
      dependencies: [
        dependency('threads'),
        gtest_proj.dependency('gtest'),
        gtest_proj.dependency('gtest_main'),
      ])
  else
    assert(not build_tests.allowed(), 'Googletest is required')
  endif
endif

test('fileio_test', executable('fileio_test',
  ['fileio_test.cpp', '../file-io.cpp'],
  implicit_include_directories: false,
  dependencies: [gtest, dependency('stdplus')]))
