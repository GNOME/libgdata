gdata_subdir = 'gdata'
gdata_include_path = gdata_includedir / gdata_subdir
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

version_conf = {
  'GDATA_VERSION_MAJOR': gdata_version_major,
  'GDATA_VERSION_MINOR': gdata_version_minor,
  'GDATA_VERSION_MICRO': gdata_version_micro,
}

headers += configure_file(
  input: 'gdata-version.h.in',
  output: '@BASENAME@',
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
    enum_header_generator_script,
    enum_header_file,
    '--replace', 'g_data', 'gdata',
    '--source-dir', meson.current_source_dir(),
    '--out-dir', meson.current_build_dir(),
    '--sources'
    ] + gdata_headers,
  install: true,
  install_dir: gdata_include_path,
  output: enum_header_file,
  console: true
)

# Generate the enum source file in the source code directory
gdata_enum_source = custom_target(
  enum_source_file,
  command: [
    python_prog,
    enum_source_generator_script,
    enum_source_file,
    '--replace', 'g_data', 'gdata',
    '--source-dir', meson.current_source_dir(),
    '--out-dir', meson.current_build_dir(),
    '--include-dir', gdata_subdir,
    '--sources'
    ] + gdata_headers,
  output: enum_source_file,
  console: true
)

marshal = 'gdata-marshal'

gdata_marshal_files = gnome.genmarshal(
  marshal,
  sources: marshal + '.list',
  prefix: marshal.underscorify(),
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

headers += files(gdata_headers) + [gdata_enum_header]
sources += gdata_enum_source

symbol_map = meson.current_source_dir() / 'symbol.map'
ldflags = cc.get_supported_link_arguments('-Wl,--version-script,' + symbol_map)

libgdata_lib = shared_library(
  'gdata',
  include_directories: top_inc_dir,
  sources: sources + headers + gdata_private_headers + gdata_marshal_files,
  dependencies: gdata_deps + gdata_private_deps,
  link_args: ldflags,
  link_depends: symbol_map,
  install: true,
  soversion: gdata_soversion,
  gnu_symbol_visibility: 'default'
)

libgdata_dep = declare_dependency(
  sources: gdata_enum_header,
  link_with: libgdata_lib,
  include_directories: gdata_inc_dir,
  dependencies: gdata_deps + gdata_private_deps
)

pkgconfig.generate(
  name: 'libgdata',
  description: 'GData client library',
  subdirs: 'libgdata',
  libraries: libgdata_lib,
  requires: gdata_deps,
  requires_private: gdata_private_deps,
  filebase: 'libgdata',
  version: gdata_version,
  variables: [
    'exec_prefix=${prefix}',
    'goa_enabled=' + goa_dep.found().to_string(),
  ],
)

if get_option('with_introspection')

  gdata_gir_includes = ['GObject-2.0', 'libxml2-2.0', 'Soup-2.4', 'Json-1.0']
  gdata_vapi_deps = ['libxml-2.0', 'libsoup-2.4', 'json-glib-1.0']

  if enable_goa.enabled()
    gdata_gir_includes += ['Goa-1.0']
    gdata_vapi_deps += ['goa-1.0']
  endif

  libgdata_gir = gnome.generate_gir(libgdata_lib,
    sources: sources + headers,
    nsversion: '@0@.@1@'.format(gdata_api_version_major, gdata_api_version_minor),
    namespace: 'GData',
    symbol_prefix: 'gdata',
    includes: gdata_gir_includes,
    link_with: libgdata_lib,
    dependencies: libgdata_dep,
    header: 'gdata/gdata.h',
    install: true,
    install_dir_gir: gdata_gir_dir,
    install_dir_typelib: gdata_typelib_dir,
    export_packages: gdata_name,
  )

  if get_option('with_vapi')

    libgdata_vapi = gnome.generate_vapi(gdata_name,
      sources: libgdata_gir[0],
      packages: gdata_vapi_deps,
      install: true,
      install_dir: gdata_vapi_dir,
    )

  endif
endif
subdir('tests')
