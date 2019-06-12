gdata_media_path = 'gdata/media'
gdata_media_include_path = join_paths(gdata_includedir, gdata_media_path)

gdata_private_headers += files(['gdata-media-group.h'])

gdata_media_headers = [
  'gdata-media-category.h',
  'gdata-media-content.h',
  'gdata-media-credit.h',
  'gdata-media-thumbnail.h',
]

install_headers(
  gdata_media_headers,
  install_dir: gdata_media_include_path
)

enum_header_file = 'gdata-media-enums.h'
enum_source_file = 'gdata-media-enums.c'

# Generate the enum source file in the source code and build directories
gdata_media_enum_header = custom_target(
  enum_header_file,
  command: [
    python_prog,
    join_paths(meson.current_source_dir(), '../../scripts/generate_enum_header.py'),
    enum_header_file,
    '--replace', 'g_data', 'gdata',
    '--replace', 'GDATA_TYPE_DATA', 'GDATA_TYPE',
    '--source-dir', meson.current_source_dir(),
    '--out-dir', meson.current_build_dir(),
    '--sources'
    ] + gdata_media_headers,
  install: true,
  install_dir: gdata_media_include_path,
  output: [enum_header_file],
  console: true
)

# Generate the enum source file in the source code directory
gdata_media_enum_source = custom_target(
  enum_source_file,
  command: [
    python_prog,
    join_paths(meson.current_source_dir(), '../../scripts/generate_enum_source.py'),
    enum_source_file,
    '--replace', 'g_data', 'gdata',
    '--source-dir', meson.current_source_dir(),
    '--out-dir', meson.current_build_dir(),
    '--sources'
    ] + gdata_media_headers,
  output: [enum_source_file],
  console: true
)

# Copy all the header files required to compile the enum_source_file
r = run_command(
  cp_prog,
  ['-r'] + gdata_media_headers + [meson.current_build_dir()],
  check: true
)

sources += files([
  'gdata-media-category.c',
  'gdata-media-content.c',
  'gdata-media-credit.c',
  'gdata-media-group.c',
  'gdata-media-thumbnail.c',
])

headers += files(gdata_media_headers)
generated_enum_headers += gdata_media_enum_header
generated_enum_sources += gdata_media_enum_source
