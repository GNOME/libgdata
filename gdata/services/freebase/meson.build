gdata_freebase_include_path = join_paths(gdata_includedir, 'gdata/services/freebase')

gdata_freebase_headers = [
  'gdata-freebase-query.h',
  'gdata-freebase-result.h',
  'gdata-freebase-search-query.h',
  'gdata-freebase-search-result.h',
  'gdata-freebase-service.h',
  'gdata-freebase-topic-query.h',
  'gdata-freebase-topic-result.h',
]

enum_header_file = 'gdata-freebase-enums.h'
enum_source_file = 'gdata-freebase-enums.c'

# Generate the enum header file in the source code directory
r = run_command([
  join_paths(meson.source_root(), 'scripts/generate_enum_header.sh'),
  '-s', 's/g_data_freebase/gdata_freebase/',
  '-s', 's/GDATA_TYPE_DATA_FREEBASE/GDATA_TYPE_FREEBASE/',
  enum_header_file] + gdata_freebase_headers)

if r.returncode() != 0
  error('Couldn\'t generate header file - ' + enum_header_file)
endif

gdata_freebase_headers += [enum_header_file]
install_headers(files(gdata_freebase_headers), install_dir: gdata_freebase_include_path)

# Generate the enum source file in the source code directory
r = run_command([
  join_paths(meson.source_root(), 'scripts/generate_enum_source.sh'),
  '-s', 's/g_data_freebase/gdata_freebase/',
  enum_source_file] + gdata_freebase_headers)

if r.returncode() != 0
  error('Couldn\'t generate source file - ' + enum_source_file)
endif
