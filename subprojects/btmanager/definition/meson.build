
btdefinition_inc = include_directories('.')

btdefinition_lib = static_library(
  'btDefinitions',
  'btDefinitions.cpp',
   include_directories: btdefinition_inc
)

btdefinition_dep = declare_dependency(
  include_directories: btdefinition_inc,
  link_with: btdefinition_lib
)
