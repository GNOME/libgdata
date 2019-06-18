gdata_youtube_subdir = 'gdata/services/youtube'
gdata_youtube_include_path = gdata_includedir / gdata_youtube_subdir

gdata_youtube_headers = [
  'gdata-youtube-category.h',
  'gdata-youtube-comment.h',
  'gdata-youtube-content.h',
  'gdata-youtube-credit.h',
  'gdata-youtube-feed.h',
  'gdata-youtube-query.h',
  'gdata-youtube-service.h',
  'gdata-youtube-state.h',
  'gdata-youtube-video.h',
]

install_headers(
  gdata_youtube_headers,
  install_dir: gdata_youtube_include_path
)

enum_header_file = 'gdata-youtube-enums.h'
enum_source_file = 'gdata-youtube-enums.c'

# Generate the enum source file in the source code and build directories
gdata_youtube_enum_header = custom_target(
  enum_header_file,
  command: [
    python_prog,
    enum_header_generator_script,
    enum_header_file,
    '--replace', 'g_data_you_tube', 'gdata_youtube',
    '--replace', 'GDATA_TYPE_DATA_YOU_TUBE', 'GDATA_TYPE_YOUTUBE',
    '--source-dir', meson.current_source_dir(),
    '--out-dir', meson.current_build_dir(),
    '--sources'
    ] + gdata_youtube_headers,
  install: true,
  install_dir: gdata_youtube_include_path,
  output: [enum_header_file],
  console: true
)

# Generate the enum source file in the source code directory
gdata_youtube_enum_source = custom_target(
  enum_source_file,
  command: [
    python_prog,
    enum_source_generator_script,
    enum_source_file,
    '--replace', 'g_data_you_tube', 'gdata_youtube',
    '--source-dir', meson.current_source_dir(),
    '--out-dir', meson.current_build_dir(),
    '--include-dir', gdata_youtube_subdir,
    '--sources'
    ] + gdata_youtube_headers,
  output: [enum_source_file],
  console: true
)

sources += files([
  'gdata-youtube-category.c',
  'gdata-youtube-comment.c',
  'gdata-youtube-content.c',
  'gdata-youtube-credit.c',
  'gdata-youtube-feed.c',
  'gdata-youtube-query.c',
  'gdata-youtube-service.c',
  'gdata-youtube-state.c',
  'gdata-youtube-video.c',
])

headers += files(gdata_youtube_headers) + [gdata_youtube_enum_header]
sources += gdata_youtube_enum_source
