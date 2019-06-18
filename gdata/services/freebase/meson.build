gdata_freebase_inc_dir = include_directories('.')

include_subdir = gdata_include_subdir / 'services/freebase'

gdata_freebase_headers = [
  'gdata-freebase-query.h',
  'gdata-freebase-result.h',
  'gdata-freebase-search-query.h',
  'gdata-freebase-search-result.h',
  'gdata-freebase-service.h',
  'gdata-freebase-topic-query.h',
  'gdata-freebase-topic-result.h',
]

install_headers(
  gdata_freebase_headers,
  subdir: include_subdir,
)

enums = 'gdata-freebase-enums'

enums_in = gnome.mkenums_simple(
  enums + '-in',
  sources: gdata_freebase_headers,
)

# workaround to fix namespace
gdata_freebase_enum_source = custom_target(
  enums + '.c',
  input: enums_in[0],
  output: enums + '.c',
  command: enum_source_cmd,
  capture: true,
)

gdata_freebase_enum_header = custom_target(
  enums + '.h',
  input: enums_in[1],
  output: enums + '.h',
  command: enum_header_cmd,
  capture: true,
  install: true,
  install_dir: gdata_includedir / include_subdir,
)

sources += files([
  'gdata-freebase-query.c',
  'gdata-freebase-result.c',
  'gdata-freebase-search-query.c',
  'gdata-freebase-search-result.c',
  'gdata-freebase-service.c',
  'gdata-freebase-topic-query.c',
  'gdata-freebase-topic-result.c',
])

headers += files(gdata_freebase_headers) + [gdata_freebase_enum_header]
sources += gdata_freebase_enum_source
