incs += include_directories('.')

include_subdir = gdata_include_subdir / 'services/youtube'

headers = files(
  'gdata-youtube-category.h',
  'gdata-youtube-comment.h',
  'gdata-youtube-feed.h',
  'gdata-youtube-query.h',
  'gdata-youtube-service.h',
  'gdata-youtube-state.h',
  'gdata-youtube-video.h',
)

install_headers(
  headers,
  subdir: include_subdir,
)

gir_headers += headers

sources += files(
  'gdata-youtube-category.c',
  'gdata-youtube-comment.c',
  'gdata-youtube-feed.c',
  'gdata-youtube-query.c',
  'gdata-youtube-service.c',
  'gdata-youtube-state.c',
  'gdata-youtube-video.c',
)

enums = 'gdata-youtube-enums'

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
  command: [sed, '-e', 's/-in//', '-e', 's/g_data_you_tube/gdata_youtube/', build_root / '@INPUT@'],
  capture: true,
)

enum_headers += custom_target(
  enums + '.h',
  input: enums_in[1],
  output: enums + '.h',
  command: [sed, '-e', 's/g_data_you_tube/gdata_youtube/', '-e', 's/G_TYPE_DATA_YOU_TUBE/GDATA_TYPE_YOUTUBE/', build_root / '@INPUT@'],
  capture: true,
  install: true,
  install_dir: gdata_includedir / include_subdir,
)
