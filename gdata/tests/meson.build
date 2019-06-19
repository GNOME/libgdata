tests_execdir = gdata_libexecdir / 'installed-tests' / gdata_name
tests_metadir = gdata_datadir / 'installed-tests' / gdata_name

tests_sources = files([
  'common.c',
  'gdata-dummy-authorizer.c',
])

tests_headers = files([
  'common.h',
  'gdata-dummy-authorizer.h',
])

# The empty objects may be used in future for keeping custom
# arguments for each test
tests = {
  'authorization': {},
  'buffer': {},
  'calendar': {},
  'contacts': {},
  'documents': {
    'should_fail': true
  },
  'general': {},
  'oauth1-authorizer': {},
  'oauth2-authorizer': {},
  'perf': {},
  'picasaweb': {
    'should_fail': true
  },
  'streams': {},
  'tasks': {},
  'youtube': {},
}

if always_build_tests
  test_env = environment()
  test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
  test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())

  foreach test_name, extra_args: tests
    should_fail = extra_args.get('should_fail', false)

    exe = executable(
      test_name,
      test_name + '.c',
      c_args: '-DG_LOG_DOMAIN="@0@"'.format(gdata_name),
      include_directories: [top_inc_dir, gdata_inc_dir],
      link_with: libgdata_lib,
      dependencies: gdata_deps + gdata_private_deps,
      sources: tests_sources + tests_headers + headers,
      install: install_tests,
      install_dir: tests_execdir
    )

    test(
      test_name,
      exe,
      env: test_env,
      should_fail: should_fail,
    )
  endforeach
endif

if always_build_tests and install_tests
  foreach test_name, extra_args: tests
    tests_conf = configuration_data ()
    tests_conf.set('TEST_TYPE', 'session')
    tests_conf.set('TEST_ABS_PATH', tests_execdir / test_name)
    config_h.set_quoted('foobar', tests_execdir)
    config_h.set_quoted('foobar', tests_execdir / test_name)

    configure_file (
      input: 'template.test.in',
      output: test_name + '.test',
      install: true,
      install_dir: tests_metadir,
      configuration: tests_conf
    )
  endforeach

  install_subdir(
    'traces',
    install_dir: tests_execdir
  )

  test_data = [
    'cert.pem',
    'key.pem',
    'photo.jpg',
    'sample.ogg',
    'test.doc',
    'test.ods',
    'test.odt',
    'test.ppt',
    'test_updated.odt',
    'test_updated_file.ppt',
    'test.xls',
  ]

  install_data(
    test_data,
    install_dir: tests_execdir,
  )
endif
