gdata_picasaweb_include_path = join_paths(gdata_includedir, 'gdata/services/picasaweb')

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
  files(gdata_picasaweb_headers),
  install_dir: gdata_picasaweb_include_path
)

enum_header_file = 'gdata-picasaweb-enums.h'
enum_source_file = 'gdata-picasaweb-enums.c'

# Generate the enum source file in the source code and build directories
gdata_picasaweb_enum_header = custom_target(
  enum_header_file,
  command: [
    python_prog,
    join_paths(meson.current_source_dir(), '../../../scripts/generate_enum_header.py'),
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
    join_paths(meson.current_source_dir(), '../../../scripts/generate_enum_source.py'),
    enum_source_file,
    '--replace', 'g_data_picasa_web', 'gdata_picasaweb',
    '--source-dir', meson.current_source_dir(),
    '--out-dir', meson.current_build_dir(),
    '--sources'
    ] + gdata_picasaweb_headers,
  output: [enum_source_file],
  console: true
)

# Copy all the header files required to compile the enum_source_file
r = run_command(
  cp_prog,
  ['-r'] + gdata_picasaweb_headers + [meson.current_build_dir()],
  check: true
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

headers += files(gdata_picasaweb_headers)
generated_enum_headers += gdata_picasaweb_enum_header
generated_enum_sources += gdata_picasaweb_enum_source
