incs += include_directories('.')

include_subdir = gdata_include_subdir / 'services/picasaweb'

headers = files(
  'gdata-picasaweb-album.h',
  'gdata-picasaweb-comment.h',
  'gdata-picasaweb-feed.h',
  'gdata-picasaweb-file.h',
  'gdata-picasaweb-query.h',
  'gdata-picasaweb-service.h',
  'gdata-picasaweb-user.h',
)

install_headers(
  headers,
  subdir: include_subdir,
)

gir_headers += headers

sources += files(
  'gdata-picasaweb-album.c',
  'gdata-picasaweb-comment.c',
  'gdata-picasaweb-feed.c',
  'gdata-picasaweb-file.c',
  'gdata-picasaweb-query.c',
  'gdata-picasaweb-service.c',
  'gdata-picasaweb-user.c',
)

enums = 'gdata-picasaweb-enums'

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
  command: [sed, '-e', 's/-in//', '-e', 's/g_data_picasa_web/gdata_picasaweb/', build_root / '@INPUT@'],
  capture: true,
)

enum_headers += custom_target(
  enums + '.h',
  input: enums_in[1],
  output: enums + '.h',
  command: [sed, '-e', 's/g_data_picasa_web/gdata_picasaweb/', '-e', 's/G_TYPE_DATA_PICASA_WEB/GDATA_TYPE_PICASAWEB/',  build_root / '@INPUT@'],
  capture: true,
  install: true,
  install_dir: gdata_includedir / include_subdir,
)
