gdata_youtube_inc_dir = include_directories('.')

include_subdir = gdata_include_subdir / 'services/youtube'

gdata_youtube_headers = files(
  'gdata-youtube-category.h',
  'gdata-youtube-comment.h',
  'gdata-youtube-content.h',
  'gdata-youtube-credit.h',
  'gdata-youtube-feed.h',
  'gdata-youtube-query.h',
  'gdata-youtube-service.h',
  'gdata-youtube-state.h',
  'gdata-youtube-video.h',
)

install_headers(
  gdata_youtube_headers,
  subdir: include_subdir,
)

enums = 'gdata-youtube-enums'

enums_in = gnome.mkenums_simple(
  enums + '-in',
  sources: gdata_youtube_headers,
)

# workaround to fix namespace
gdata_youtube_enum_source = custom_target(
  enums + '.c',
  input: enums_in[0],
  output: enums + '.c',
  command: [sed, '-e', 's/-in//', '-e', 's/g_data_you_tube/gdata_youtube/', build_root / '@INPUT@'],
  capture: true,
)

gdata_youtube_enum_header = custom_target(
  enums + '.h',
  input: enums_in[1],
  output: enums + '.h',
  command: [sed, '-e', 's/g_data_you_tube/gdata_youtube/', '-e', 's/G_TYPE_DATA_YOU_TUBE/GDATA_TYPE_YOUTUBE/', build_root / '@INPUT@'],
  capture: true,
  install: true,
  install_dir: gdata_includedir / include_subdir,
)

sources += files(
  'gdata-youtube-category.c',
  'gdata-youtube-comment.c',
  'gdata-youtube-content.c',
  'gdata-youtube-credit.c',
  'gdata-youtube-feed.c',
  'gdata-youtube-query.c',
  'gdata-youtube-service.c',
  'gdata-youtube-state.c',
  'gdata-youtube-video.c',
)

headers += gdata_youtube_headers + [gdata_youtube_enum_header]
sources += gdata_youtube_enum_source
