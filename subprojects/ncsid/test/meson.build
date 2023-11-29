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
    gmock = gtest_proj.dependency('gmock')
  else
    assert(not build_tests.allowed(), 'Googletest is required')
  endif
endif

tests = [
  'iface_test',
  'sock_test',
  #'ncsi_test',  # TODO: Re-enable when fixed
]

ncsid_test_headers = include_directories('.')

ncsid_test_lib = static_library(
  'ncsid_test',
  [
    'net_iface_mock.cpp',
    'nic_mock.cpp',
  ],
  include_directories: ncsid_test_headers,
  implicit_include_directories: false,
  dependencies: ncsid)

ncsid_test = declare_dependency(
  dependencies: ncsid,
  include_directories: ncsid_test_headers,
  link_with: ncsid_test_lib)

foreach t : tests
  test(t, executable(t.underscorify(), t + '.cpp',
                     implicit_include_directories: false,
                     dependencies: [gtest, gmock, ncsid_test]))
endforeach

script_tests = [
  'normalize_ip_test',
  'normalize_mac_test',
]

script_env = environment()
script_deps = []
script_env.set('NORMALIZE_IP', normalize_ip.full_path())
script_deps += normalize_ip
script_env.set('NORMALIZE_MAC', normalize_mac.full_path())
script_deps += normalize_mac

foreach st : script_tests
  test(st, find_program('bash'), args: files(st + '.sh'),
    protocol: 'tap', env: script_env, depends: script_deps)
endforeach
