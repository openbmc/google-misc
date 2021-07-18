tests = [
  'hoth_command_unittest',
  'hoth_command_close_unittest',
  'hoth_command_commit_unittest',
  'hoth_command_delete_unittest',
  'hoth_command_open_unittest',
  'hoth_command_read_unittest',
  'hoth_command_sessionstat_unittest',
  'hoth_command_write_unittest',
]

foreach t : tests
  test(
    t,
    executable(
      t.underscorify(),
      t + '.cpp',
      implicit_include_directories: false,
      dependencies: [hothcommand_dep, test_dep]))
endforeach
