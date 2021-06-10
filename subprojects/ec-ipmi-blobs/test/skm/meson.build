tests = [
  'cmd',
  'handler',
]

foreach t : tests
  test(
    'skm-' + t,
    executable(
      t.underscorify(),
      t + '.cpp',
      implicit_include_directories: false,
      dependencies: [ecskmblob_dep, gtest, gmock]))
endforeach
