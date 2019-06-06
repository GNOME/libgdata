gdata_path = 'gdata'
gdata_include_path = join_paths(gdata_includedir, gdata_path)

subdir('app')
subdir('atom')
subdir('exif')
subdir('gcontact')
subdir('gd')
subdir('georss')
subdir('media')
subdir('services')
# TODO: subdir('tests')

gdata_private_headers += files([
  'gdata-private.h',
  'gdata-batch-private.h',
  'gdata-batch-feed.h',
  'gdata-parser.h',
  'gdata-buffer.h',
])

gdata_headers = [
  'gdata.h',
  'gdata-entry.h',
  'gdata-feed.h',
  'gdata-service.h',
  'gdata-query.h',
  'gdata-commentable.h',
  'gdata-comment.h',
  'gdata-access-handler.h',
  'gdata-access-rule.h',
  'gdata-parsable.h',
  'gdata-download-stream.h',
  'gdata-upload-stream.h',
  'gdata-comparable.h',
  'gdata-batch-operation.h',
  'gdata-batchable.h',
  'gdata-authorizer.h',
  'gdata-authorization-domain.h',
  'gdata-client-login-authorizer.h',
  'gdata-oauth1-authorizer.h',
  'gdata-oauth2-authorizer.h',
]

# TODO: Handle these separately later
# 'gdata-types.h',
# 'gdata-version.h'

if enable_goa.enabled()
  gdata_headers += ['gdata-goa-authorizer.h']
  sources += files(['gdata-goa-authorizer.c'])
endif

install_headers(files(gdata_headers), install_dir: gdata_include_path)

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

# Copy all the header files required to compile the enum_source_file
r = run_command(
  cp_prog,
  ['-r'] + gdata_headers + [meson.current_build_dir()],
  check: true
)

sources += files([
  'gdata-entry.c',
  'gdata-feed.c',
  'gdata-service.c',
  'gdata-types.c',
  'gdata-query.c',
  'gdata-parser.c',
  'gdata-commentable.c',
  'gdata-comment.c',
  'gdata-access-handler.c',
  'gdata-access-rule.c',
  'gdata-parsable.c',
  'gdata-download-stream.c',
  'gdata-upload-stream.c',
  'gdata-buffer.c',
  'gdata-comparable.c',
  'gdata-batch-operation.c',
  'gdata-batchable.c',
  'gdata-batch-feed.c',
  'gdata-authorizer.c',
  'gdata-authorization-domain.c',
  'gdata-client-login-authorizer.c',
  'gdata-oauth1-authorizer.c',
  'gdata-oauth2-authorizer.c'
])

headers += files(gdata_headers)
generated_enum_headers += gdata_enum_header
generated_enum_sources += gdata_enum_source
marshal_files += gdata_marshal_files

libgdata = shared_library(
  'gdata',
  include_directories: top_inc,
  sources: sources + headers + generated_enum_headers +
    generated_enum_sources + marshal_files,
  dependencies: gdata_deps,
  install: true,
  soversion: gdata_soversion,
  gnu_symbol_visibility: 'default'
)
