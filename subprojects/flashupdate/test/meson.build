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
  endif
endif

flashupdate_args_test = executable(
  'flashupdate_args',
  'args.cpp',
  build_by_default: false,
  implicit_include_directories: false,
  dependencies: [libflashupdate, gtest, gmock])
test('flashupdate/args', flashupdate_args_test)


flashupdate_config_test = executable(
  'flashupdate_config',
  'config.cpp',
  build_by_default: false,
  implicit_include_directories: false,
  dependencies: [libflashupdate, gtest, gmock])
test('flashupdate/config', flashupdate_config_test)

flashupdate_flash_test = executable(
  'flashupdate_flash',
  'flash.cpp',
  build_by_default: false,
  implicit_include_directories: false,
  dependencies: [libflashupdate, gtest, gmock])
test('flashupdate/flash', flashupdate_flash_test)

if get_option('tests').enabled()
  assert(not is_disabler(flashupdate_args_test),
         'Tests enabled but flashupdate_args_test cannot build')
  assert(not is_disabler(flashupdate_config_test),
        'Tests enabled but flashupdate_config_test cannot build')
  assert(not is_disabler(flashupdate_flash_test),
        'Tests enabled but flashupdate_flash_test cannot build')
endif
