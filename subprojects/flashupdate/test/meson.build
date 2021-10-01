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

flasheupdate_args_test = executable(
  'flashupdate_args',
  'args.cpp',
  build_by_default: false,
  implicit_include_directories: false,
  dependencies: [libflashupdate, gtest, gmock])
test('flasheupdate/args', flasheupdate_args_test)


flasheupdat_config_test = executable(
  'flashupdate_config',
  'config.cpp',
  build_by_default: false,
  implicit_include_directories: false,
  dependencies: [libflashupdate, gtest, gmock])
test('flasheupdate/config', flasheupdat_config_test)

if get_option('tests').enabled()
  assert(not is_disabler(flasheupdate_args_test),
         'Tests enabled but flasheupdate_args_test cannot build')
  assert(not is_disabler(flasheupdat_config_test),
         'Tests enabled but flasheupdat_config_test cannot build')
endif
