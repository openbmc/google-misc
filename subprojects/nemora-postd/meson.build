project(
  'nemora-postd',
  'cpp',
  version: '0.1',
  default_options: [
    'warning_level=3',
    'werror=true',
    'cpp_std=c++17'
  ],
)

protobuf = dependency('protobuf')
phosphor_dbus_interfaces = dependency('phosphor-dbus-interfaces')
sdbusplus = dependency('sdbusplus')
threads = dependency('threads')
logging = dependency('phosphor-logging')
fmt = declare_dependency(compile_args: '-DFMT_STRING_ALIAS=1', dependencies: dependency('fmt'))

protoc = find_program('protoc', native: true)

proto = custom_target(
  'event_message_proto',
  command: [
    find_program('protoc', native: true),
    '--proto_path=@CURRENT_SOURCE_DIR@',
    '--cpp_out=@OUTDIR@',
    '@INPUT@'
  ],
  output: [
    'event_message.pb.cc',
    'event_message.pb.h',
  ],
  input: 'event_message.proto')

executable(
  'nemora-postd',
  'nemorad.cpp',
  'src/host_manager.cpp',
  'src/nemora.cpp',
  'src/socket_manager.cpp',
  'src/serializer.cpp',
  proto,
  include_directories: include_directories('.'),
  dependencies: [protobuf, phosphor_dbus_interfaces, sdbusplus, threads, logging, fmt],
  install: true
)


