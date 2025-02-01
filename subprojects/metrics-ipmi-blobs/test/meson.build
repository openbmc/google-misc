gtest = dependency('gtest', main: true, disabler: true, required: false)
gmock = dependency('gmock', disabler: true, required: false)
if not gtest.found() or not gmock.found()
    gtest_proj = import('cmake').subproject(
        'googletest',
        cmake_options: ['-DCMAKE_CXX_FLAGS=-Wno-pedantic'],
        required: false,
    )
    if gtest_proj.found()
        gtest = declare_dependency(
            dependencies: [
                dependency('threads'),
                gtest_proj.dependency('gtest'),
                gtest_proj.dependency('gtest_main'),
            ],
        )
        gmock = gtest_proj.dependency('gmock')
    else
        assert(not get_option('tests').allowed(), 'Googletest is required')
    endif
endif

tests = ['util_test']

foreach t : tests
    test(
        t,
        executable(
            t.underscorify(),
            t + '.cpp',
            implicit_include_directories: false,
            dependencies: [gtest, gmock, dep],
        ),
    )
endforeach

