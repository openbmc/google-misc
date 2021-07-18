gtest = dependency('gtest', main: true, disabler: true, required: get_option('tests'))
gmock = dependency('gmock', disabler: true, required: get_option('tests'))

test_dep = declare_dependency(
  include_directories: include_directories('.'),
  dependencies: [gtest, gmock])
