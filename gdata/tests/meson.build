tests_execdir = gdata_libexecdir / 'installed-tests' / gdata_name
tests_metadir = gdata_datadir / 'installed-tests' / gdata_name

tests_sources = files(
  'common.c',
  'gdata-dummy-authorizer.c',
)

common_deps = [
  libgdata_dep,
  libuhttpmock_dep,
]

# The empty objects may be used in future for keeping custom
# arguments for each test
tests = {
  'authorization': {},
  'buffer': {},
  'calendar': {},
  'documents': {
    'dependencies': [gdk_pixbuf_dep],
    'should_fail': true,
  },
  'general': {},
  'oauth2-authorizer': {},
  'perf': {},
  'picasaweb': {
    'dependencies': [gdk_pixbuf_dep],
    'should_fail': true,
  },
  'streams': {},
  'tasks': {},
  'youtube': {},
}

test_env = environment()
test_env.set('G_TEST_SRCDIR', meson.current_source_dir())
test_env.set('G_TEST_BUILDDIR', meson.current_build_dir())

foreach test_name, extra_args: tests
  should_fail = extra_args.get('should_fail', false)
  exe = executable(
    test_name,
    test_name + '.c',
    c_args: common_c_args + ['-DG_LOG_DOMAIN="@0@"'.format(gdata_name)],
    include_directories: top_inc_dir,
    dependencies: common_deps + extra_args.get('dependencies', []),
    sources: tests_sources,
    install: install_tests and not should_fail,
    install_dir: tests_execdir,
  )

  test(
    test_name,
    exe,
    env: test_env,
    should_fail: should_fail,
  )
endforeach

if install_tests
  foreach test_name, extra_args: tests
    should_fail = extra_args.get('should_fail', false)
    tests_conf = {
      'TEST_TYPE': 'session',
      'TEST_ABS_PATH': gdata_prefix / tests_execdir / test_name,
    }

    configure_file (
      input: 'template.test.in',
      output: test_name + '.test',
      configuration: tests_conf,
      install: not should_fail,
      install_dir: tests_metadir,
    )
  endforeach

  install_subdir(
    'traces',
    install_dir: tests_execdir,
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
