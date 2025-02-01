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
    'nemora-postd',
    'cpp',
    version: '0.1',
    default_options: ['warning_level=3', 'werror=true', 'cpp_std=c++23'],
)

protobuf = dependency('protobuf')
phosphor_dbus_interfaces = dependency('phosphor-dbus-interfaces')
sdbusplus = dependency('sdbusplus')
threads = dependency('threads')
logging = dependency('phosphor-logging')

protoc = find_program('protoc', native: true)

proto = custom_target(
    'event_message_proto',
    command: [
        find_program('protoc', native: true),
        '--proto_path=@CURRENT_SOURCE_DIR@',
        '--cpp_out=@OUTDIR@',
        '@INPUT@',
    ],
    output: ['event_message.pb.cc', 'event_message.pb.h'],
    input: 'event_message.proto',
)

executable(
    'nemora-postd',
    'nemorad.cpp',
    'src/host_manager.cpp',
    'src/nemora.cpp',
    'src/socket_manager.cpp',
    'src/serializer.cpp',
    proto,
    include_directories: include_directories('.'),
    dependencies: [
        protobuf,
        phosphor_dbus_interfaces,
        sdbusplus,
        threads,
        logging,
    ],
    install: true,
)


