# Copyright 2022 Google LLC
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

ipmi_blob_dep = dependency('phosphor-ipmi-blobs', required : false)
if not ipmi_blob_dep.found()
    ipmi_blob_proj = subproject('phosphor-ipmi-blobs')
    ipmi_blob_dep = ipmi_blob_proj.get_variable('ipmi_blob_dep')
endif
protobuf_dep = dependency('protobuf')

proto = custom_target(
    'hostboottime_proto',
    command: [
        find_program('protoc', native: true),
        '--proto_path=@CURRENT_SOURCE_DIR@',
        '--cpp_out=@OUTDIR@',
        '@INPUT@'
    ],
    output: [
        'hostboottime.pb.cc',
        'hostboottime.pb.h',
    ],
    input: 'hostboottime.proto'
)

shared_module(
    'boottimeblob',
    'blobmain.cpp',
    'blobHandler.cpp',
    proto,
    dependencies: [
        nlohmann_json_dep,
        ipmi_blob_dep,
        protobuf_dep,
    ],
    include_directories : ['.'],
    install: true,
    install_dir: get_option('libdir') / 'blob-ipmid'
)
