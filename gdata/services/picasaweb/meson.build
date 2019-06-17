gdata_picasaweb_inc_dir = include_directories('.')

gdata_picasaweb_subdir = 'gdata/services/picasaweb'
gdata_picasaweb_include_path = gdata_includedir / gdata_picasaweb_subdir

gdata_picasaweb_headers = [
  'gdata-picasaweb-album.h',
  'gdata-picasaweb-comment.h',
  'gdata-picasaweb-feed.h',
  'gdata-picasaweb-file.h',
  'gdata-picasaweb-query.h',
  'gdata-picasaweb-service.h',
  'gdata-picasaweb-user.h',
]

install_headers(
  gdata_picasaweb_headers,
  install_dir: gdata_picasaweb_include_path
)

enums = 'gdata-picasaweb-enums'

enums_in = gnome.mkenums_simple(
  enums + '-in',
  sources: gdata_picasaweb_headers,
)

# workaround to fix namespace
gdata_picasaweb_enum_source = custom_target(
  enums + '.c',
  input: enums_in[0],
  output: enums + '.c',
  command: [sed, '-e', 's/-in//', '-e', 's/g_data_picasa_web/gdata_picasaweb/', build_root / '@INPUT@'],
  capture: true,
)

gdata_picasaweb_enum_header = custom_target(
  enums + '.h',
  input: enums_in[1],
  output: enums + '.h',
  command: [sed, '-e', 's/g_data_picasa_web/gdata_picasaweb/', '-e', 's/G_TYPE_DATA_PICASA_WEB/GDATA_TYPE_PICASAWEB/',  build_root / '@INPUT@'],
  capture: true,
  install: true,
  install_dir: gdata_picasaweb_include_path,
)

sources += files([
  'gdata-picasaweb-album.c',
  'gdata-picasaweb-comment.c',
  'gdata-picasaweb-feed.c',
  'gdata-picasaweb-file.c',
  'gdata-picasaweb-query.c',
  'gdata-picasaweb-service.c',
  'gdata-picasaweb-user.c',
])

headers += files(gdata_picasaweb_headers) + [gdata_picasaweb_enum_header]
sources += gdata_picasaweb_enum_source
