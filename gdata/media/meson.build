incs += include_directories('.')

include_subdir = gdata_include_subdir / 'media'

headers = files(
  'gdata-media-category.h',
  'gdata-media-content.h',
  'gdata-media-credit.h',
  'gdata-media-thumbnail.h',
)

install_headers(
  headers,
  subdir: include_subdir,
)

gir_headers += headers

sources += files(
  'gdata-media-category.c',
  'gdata-media-content.c',
  'gdata-media-credit.c',
  'gdata-media-group.c',
  'gdata-media-thumbnail.c',
)

enums = 'gdata-media-enums'

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
