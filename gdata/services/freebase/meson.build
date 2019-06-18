gdata_freebase_subdir = 'gdata/services/freebase'
gdata_freebase_include_path = gdata_includedir / gdata_freebase_subdir

gdata_freebase_headers = [
  'gdata-freebase-query.h',
  'gdata-freebase-result.h',
  'gdata-freebase-search-query.h',
  'gdata-freebase-search-result.h',
  'gdata-freebase-service.h',
  'gdata-freebase-topic-query.h',
  'gdata-freebase-topic-result.h',
]

install_headers(
  gdata_freebase_headers,
  install_dir: gdata_freebase_include_path
)

enum_header_file = 'gdata-freebase-enums.h'
enum_source_file = 'gdata-freebase-enums.c'

# Generate the enum source file in the source code and build directories
gdata_freebase_enum_header = custom_target(
  enum_header_file,
  command: [
    python_prog,
    enum_header_generator_script,
    enum_header_file,
    '--replace', 'g_data_freebase', 'gdata_freebase',
    '--replace', 'GDATA_TYPE_DATA_FREEBASE', 'GDATA_TYPE_FREEBASE',
    '--source-dir', meson.current_source_dir(),
    '--out-dir', meson.current_build_dir(),
    '--sources'
    ] + gdata_freebase_headers,
  install: true,
  install_dir: gdata_freebase_include_path,
  output: [enum_header_file],
  console: true
)

# Generate the enum source file in the source code directory
gdata_freebase_enum_source = custom_target(
  enum_source_file,
  command: [
    python_prog,
    enum_source_generator_script,
    enum_source_file,
    '--replace', 'g_data_freebase', 'gdata_freebase',
    '--source-dir', meson.current_source_dir(),
    '--out-dir', meson.current_build_dir(),
    '--include-dir', gdata_freebase_subdir,
    '--sources'
    ] + gdata_freebase_headers,
  input: [gdata_freebase_enum_header],
  output: [enum_source_file],
  console: true
)

sources += files([
  'gdata-freebase-query.c',
  'gdata-freebase-result.c',
  'gdata-freebase-search-query.c',
  'gdata-freebase-search-result.c',
  'gdata-freebase-service.c',
  'gdata-freebase-topic-query.c',
  'gdata-freebase-topic-result.c',
])

headers += files(gdata_freebase_headers) + [gdata_freebase_enum_header]
sources += gdata_freebase_enum_source
