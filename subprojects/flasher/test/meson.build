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

gtest = dependency('gtest', main: true, disabler: true, required: false)
gmock = dependency('gmock', disabler: true, required: false)
if not gtest.found() or not gmock.found()
  gtest_opts = import('cmake').subproject_options()
  gtest_opts.add_cmake_defines({'CMAKE_CXX_FLAGS': '-Wno-pedantic'})
  gtest_proj = import('cmake').subproject(
    'googletest',
    options: gtest_opts,
    required: false)
  if gtest_proj.found()
    gtest = declare_dependency(
      dependencies: [
        dependency('threads'),
        gtest_proj.dependency('gtest'),
        gtest_proj.dependency('gtest_main'),
      ])
    gmock = gtest_proj.dependency('gmock')
  else
    assert(not get_option('tests').enabled(), 'Googletest is required')
  endif
endif

test_pre = declare_dependency(
  include_directories: flasher_internal_includes,
  dependencies: [gtest, gmock, flasher_dep])

test_dep = declare_dependency(
  dependencies: test_pre)

tests = [
  'args',
  'convert',
  'device',
  'device/mutated',
  'file',
  'file/memory',
  'file/simple',
  'mod',
  'util',
]

foreach t : tests
  test(
    t,
    executable(
      t.underscorify(),
      t + '.cpp',
      implicit_include_directories: false,
      dependencies: test_dep))
endforeach
