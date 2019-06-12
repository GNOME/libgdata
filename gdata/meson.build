gdata_path = 'gdata'
gdata_include_path = join_paths(gdata_includedir, gdata_path)
gdata_inc_dir = include_directories('.')

subdir('app')
subdir('atom')
subdir('exif')
subdir('gcontact')
subdir('gd')
subdir('georss')
subdir('media')
subdir('services')

gdata_private_headers += files([
  'gdata-batch-feed.h',
  'gdata-batch-private.h',
  'gdata-buffer.h',
  'gdata-parser.h',
  'gdata-private.h',
])

gdata_headers = [
  'gdata.h',
  'gdata-access-handler.h',
  'gdata-access-rule.h',
  'gdata-authorization-domain.h',
  'gdata-authorizer.h',
  'gdata-batch-operation.h',
  'gdata-batchable.h',
  'gdata-client-login-authorizer.h',
  'gdata-comment.h',
  'gdata-commentable.h',
  'gdata-comparable.h',
  'gdata-download-stream.h',
  'gdata-entry.h',
  'gdata-feed.h',
  'gdata-oauth1-authorizer.h',
  'gdata-oauth2-authorizer.h',
  'gdata-parsable.h',
  'gdata-query.h',
  'gdata-service.h',
  'gdata-types.h',
  'gdata-upload-stream.h',
]

version_conf = configuration_data()

version_conf.set('GDATA_VERSION_MAJOR', gdata_version_major)
version_conf.set('GDATA_VERSION_MINOR', gdata_version_minor)
version_conf.set('GDATA_VERSION_MICRO', gdata_version_micro)

version_header_name = 'gdata-version.h'

headers += configure_file(
  input: version_header_name + '.in',
  output: version_header_name,
  configuration: version_conf,
  install: true,
  install_dir: gdata_include_path,
)

if enable_goa.enabled()
  gdata_headers += ['gdata-goa-authorizer.h']
  sources += files(['gdata-goa-authorizer.c'])
endif

install_headers(gdata_headers, install_dir: gdata_include_path)

enum_header_file = 'gdata-enums.h'
enum_source_file = 'gdata-enums.c'

# Generate the enum source file in the source code and build directories
gdata_enum_header = custom_target(
  enum_header_file,
  command: [
    python_prog,
    join_paths(meson.current_source_dir(), '../scripts/generate_enum_header.py'),
    enum_header_file,
    '--replace', 'g_data', 'gdata',
    '--source-dir', meson.current_source_dir(),
    '--out-dir', meson.current_build_dir(),
    '--sources'
    ] + gdata_headers,
  install: true,
  install_dir: gdata_include_path,
  output: [enum_header_file],
  console: true
)

# Generate the enum source file in the source code directory
gdata_enum_source = custom_target(
  enum_source_file,
  command: [
    python_prog,
    join_paths(meson.current_source_dir(), '../scripts/generate_enum_source.py'),
    enum_source_file,
    '--replace', 'g_data', 'gdata',
    '--source-dir', meson.current_source_dir(),
    '--out-dir', meson.current_build_dir(),
    '--sources'
    ] + gdata_headers,
  output: [enum_source_file],
  console: true
)

gdata_marshal_files = gnome.genmarshal(
  'gdata-marshal',
  sources: 'gdata-marshal.list',
  prefix: 'gdata_marshal'
)
sources += gdata_marshal_files[0]
headers += gdata_marshal_files[1]

# Copy all the header files required to compile the enum_source_file
r = run_command(
  cp_prog,
  ['-r'] + gdata_headers + [meson.current_build_dir()],
  check: true
)

sources += files([
  'gdata-access-handler.c',
  'gdata-access-rule.c',
  'gdata-authorization-domain.c',
  'gdata-authorizer.c',
  'gdata-batch-feed.c',
  'gdata-batch-operation.c',
  'gdata-batchable.c',
  'gdata-buffer.c',
  'gdata-client-login-authorizer.c',
  'gdata-comment.c',
  'gdata-commentable.c',
  'gdata-comparable.c',
  'gdata-download-stream.c',
  'gdata-entry.c',
  'gdata-feed.c',
  'gdata-oauth1-authorizer.c',
  'gdata-oauth2-authorizer.c',
  'gdata-parsable.c',
  'gdata-parser.c',
  'gdata-query.c',
  'gdata-service.c',
  'gdata-types.c',
  'gdata-upload-stream.c',
])

headers += files(gdata_headers)
generated_enum_headers += gdata_enum_header
generated_enum_sources += gdata_enum_source

libgdata_lib = shared_library(
  'gdata',
  include_directories: top_inc_dir,
  sources: sources +
           headers +
           gdata_private_headers +
           generated_enum_headers +
           generated_enum_sources,
  dependencies: gdata_deps + gdata_private_deps,
  link_args: ldflags,
  link_depends: symbol_map,
  install: true,
  soversion: gdata_soversion,
  gnu_symbol_visibility: 'default'
)

compiled_libs += libgdata_lib

pkgconfig.generate(
  name: 'libgdata',
  description: 'GData client library',
  subdirs : ['libgdata'],
  libraries: [libgdata_lib],
  requires: gdata_deps,
  requires_private: gdata_private_deps,
  filebase : 'libgdata',
  version: gdata_version,
  variables : ['exec_prefix=${prefix}',
               'goa_enabled=' + (goa_dep.found() ? 'true': 'false')],
)

subdir('tests')
