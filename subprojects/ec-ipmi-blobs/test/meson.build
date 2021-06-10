gtest = dependency('gtest', main: true, disabler: true, required: get_option('tests'))
gmock = dependency('gmock', disabler: true, required: get_option('tests'))

subdir('skm')

tests = [
  'cmd_net',
  'io_uring',
  'util',
]

foreach t : tests
  test(
    t,
    executable(
      t.underscorify(),
      t + '.cpp',
      implicit_include_directories: false,
      dependencies: [ecblob_dep, gtest, gmock]))
endforeach
