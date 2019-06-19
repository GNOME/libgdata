gdata_media_inc_dir = include_directories('.')

include_subdir = gdata_include_subdir / 'media'

gdata_media_headers = [
  'gdata-media-category.h',
  'gdata-media-content.h',
  'gdata-media-credit.h',
  'gdata-media-thumbnail.h',
]

install_headers(
  gdata_media_headers,
  subdir: include_subdir,
)

enums = 'gdata-media-enums'

enums_in = gnome.mkenums_simple(
  enums + '-in',
  sources: gdata_media_headers,
)

# workaround to fix namespace
gdata_media_enum_source = custom_target(
  enums + '.c',
  input: enums_in[0],
  output: enums + '.c',
  command: enum_source_cmd,
  capture: true,
)

gdata_media_enum_header = custom_target(
  enums + '.h',
  input: enums_in[1],
  output: enums + '.h',
  command: enum_header_cmd,
  capture: true,
  install: true,
  install_dir: gdata_includedir / include_subdir,
)

sources += files([
  'gdata-media-category.c',
  'gdata-media-content.c',
  'gdata-media-credit.c',
  'gdata-media-group.c',
  'gdata-media-thumbnail.c',
])

headers += files(gdata_media_headers) + [gdata_media_enum_header]
sources += gdata_media_enum_source
