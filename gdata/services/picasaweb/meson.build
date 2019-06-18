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

enum_header_file = 'gdata-picasaweb-enums.h'
enum_source_file = 'gdata-picasaweb-enums.c'

# Generate the enum source file in the source code and build directories
gdata_picasaweb_enum_header = custom_target(
  enum_header_file,
  command: [
    python_prog,
    enum_header_generator_script,
    enum_header_file,
    '--replace', 'g_data_picasa_web', 'gdata_picasaweb',
    '--replace', 'GDATA_TYPE_DATA_PICASA_WEB', 'GDATA_TYPE_PICASAWEB',
    '--source-dir', meson.current_source_dir(),
    '--out-dir', meson.current_build_dir(),
    '--sources'
    ] + gdata_picasaweb_headers,
  install: true,
  install_dir: gdata_picasaweb_include_path,
  output: [enum_header_file],
  console: true
)

# Generate the enum source file in the source code directory
gdata_picasaweb_enum_source = custom_target(
  enum_source_file,
  command: [
    python_prog,
    enum_source_generator_script,
    enum_source_file,
    '--replace', 'g_data_picasa_web', 'gdata_picasaweb',
    '--source-dir', meson.current_source_dir(),
    '--out-dir', meson.current_build_dir(),
    '--include-dir', gdata_picasaweb_subdir,
    '--sources'
    ] + gdata_picasaweb_headers,
  output: [enum_source_file],
  console: true
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
