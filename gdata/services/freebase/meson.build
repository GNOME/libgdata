incs += include_directories('.')

include_subdir = gdata_include_subdir / 'services/freebase'

headers = files(
  'gdata-freebase-query.h',
  'gdata-freebase-result.h',
  'gdata-freebase-search-query.h',
  'gdata-freebase-search-result.h',
  'gdata-freebase-service.h',
  'gdata-freebase-topic-query.h',
  'gdata-freebase-topic-result.h',
)

install_headers(
  headers,
  subdir: include_subdir,
)

gir_headers += headers

sources += files(
  'gdata-freebase-query.c',
  'gdata-freebase-result.c',
  'gdata-freebase-search-query.c',
  'gdata-freebase-search-result.c',
  'gdata-freebase-service.c',
  'gdata-freebase-topic-query.c',
  'gdata-freebase-topic-result.c',
)

enums = 'gdata-freebase-enums'

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
  install_dir: gdata_includedir / include_subdir,
)
