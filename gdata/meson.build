gdata_inc_dir = include_directories('.')

enum_headers = []
gir_headers = []

sources = []

incs = [
  gdata_inc_dir,
  top_inc_dir,
]

subdir('app')
subdir('atom')
subdir('exif')
subdir('gcontact')
subdir('gd')
subdir('georss')
subdir('media')
subdir('services')

headers = files(
  'gdata.h',
  'gdata-access-handler.h',
  'gdata-access-rule.h',
  'gdata-authorization-domain.h',
  'gdata-authorizer.h',
  'gdata-batch-operation.h',
  'gdata-batchable.h',
  'gdata-comment.h',
  'gdata-commentable.h',
  'gdata-comparable.h',
  'gdata-download-stream.h',
  'gdata-entry.h',
  'gdata-feed.h',
  'gdata-oauth2-authorizer.h',
  'gdata-parsable.h',
  'gdata-query.h',
  'gdata-service.h',
  'gdata-types.h',
  'gdata-upload-stream.h',
)

sources += files(
  'gdata-access-handler.c',
  'gdata-access-rule.c',
  'gdata-authorization-domain.c',
  'gdata-authorizer.c',
  'gdata-batch-feed.c',
  'gdata-batch-operation.c',
  'gdata-batchable.c',
  'gdata-buffer.c',
  'gdata-comment.c',
  'gdata-commentable.c',
  'gdata-comparable.c',
  'gdata-download-stream.c',
  'gdata-entry.c',
  'gdata-feed.c',
  'gdata-oauth2-authorizer.c',
  'gdata-parsable.c',
  'gdata-parser.c',
  'gdata-query.c',
  'gdata-service.c',
  'gdata-types.c',
  'gdata-upload-stream.c',
)

if enable_goa
  headers += files('gdata-goa-authorizer.h')
  sources += files('gdata-goa-authorizer.c')
endif

install_headers(
  headers,
  subdir: gdata_include_subdir,
)

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
  install_dir: gdata_includedir / gdata_include_subdir,
)

gir_headers += headers

marshal = 'gdata-marshal'

marshal_files = gnome.genmarshal(
  marshal,
  sources: marshal + '.list',
  prefix: marshal.underscorify(),
)

enums = 'gdata-enums'

enums_in = gnome.mkenums_simple(
  enums + '-in',
  sources: headers,
)

# FIXME: Work around the namespace being incorrectly detected
# by glib-mkenums. This needs to be fixed by changing the
# namespace in libgdata. See !6.
sources += custom_target(
  enums + '.c',
  input: enums_in[0],
  output: enums + '.c',
  command: enum_source_cmd,
  capture: true,
)

enum_headers += custom_target(
  enums + '.h',
  input: enums_in[1],
  output: enums + '.h',
  command: enum_header_cmd,
  capture: true,
  install: true,
  install_dir: gdata_includedir / gdata_include_subdir,
)

symbol_map = meson.current_source_dir() / 'symbol.map'
ldflags = cc.get_supported_link_arguments('-Wl,--version-script,' + symbol_map)

libgdata_lib = shared_library(
  'gdata',
  include_directories: incs,
  sources: sources + enum_headers + marshal_files,
  c_args: common_c_args + ['-DG_LOG_DOMAIN="@0@"'.format(gdata_name)],
  dependencies: gdata_deps + gdata_private_deps,
  link_args: ldflags,
  link_depends: symbol_map,
  install: true,
  version: gdata_soversion,
  gnu_symbol_visibility: 'default',
)

libgdata_dep = declare_dependency(
  sources: enum_headers,
  include_directories: gdata_inc_dir,
  dependencies: gdata_deps,
  link_with: libgdata_lib,
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

if get_option('introspection')
  gdata_gir_includes = ['GObject-2.0', 'libxml2-2.0', 'Soup-2.4', 'Json-1.0']
  gdata_vapi_deps = ['libxml-2.0', 'libsoup-2.4', 'json-glib-1.0']

  if enable_goa
    gdata_gir_includes += ['Goa-1.0']
    gdata_vapi_deps += ['goa-1.0']
  endif

  libgdata_gir = gnome.generate_gir(
    libgdata_lib,
    sources: sources + gir_headers + enum_headers,
    nsversion: '@0@.@1@'.format(gdata_api_version_major, gdata_api_version_minor),
    namespace: 'GData',
    symbol_prefix: 'gdata',
    includes: gdata_gir_includes,
    header: 'gdata/gdata.h',
    install: true,
    export_packages: gdata_name,
  )

  if get_option('vapi')
    libgdata_vapi = gnome.generate_vapi(gdata_name,
      sources: libgdata_gir[0],
      packages: gdata_vapi_deps,
      install: true,
    )
  endif
endif


if always_build_tests
  subdir('tests')
endif
