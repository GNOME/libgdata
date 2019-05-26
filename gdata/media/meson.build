gdata_media_path = 'gdata/media'
gdata_media_include_path = join_paths(gdata_includedir, gdata_media_path)

gdata_media_headers = [
  'gdata-media-category.h',
  'gdata-media-content.h',
  'gdata-media-credit.h',
  'gdata-media-thumbnail.h',
]

gdata_media_private_headers = ['gdata-media-group.h']

enum_header_file = 'gdata-media-enums.h'
enum_source_file = 'gdata-media-enums.c'

# Generate the enum header file in the source code directory
r = run_command([
  join_paths(meson.source_root(), 'scripts/generate_enum_header.sh'),
  '-s', 's/g_data/gdata/',
  '-s', 's/GDATA_TYPE_DATA/GDATA_TYPE/',
  enum_header_file] + gdata_media_headers)

if r.returncode() != 0
  error('Couldn\'t generate header file - ' + enum_header_file)
endif

gdata_media_headers += [enum_header_file]
install_headers(files(gdata_media_headers), install_dir: gdata_media_include_path)

# Generate the enum source file in the source code directory
r = run_command([
  join_paths(meson.source_root(), 'scripts/generate_enum_source.sh'),
  '-s', 's/g_data/gdata/',
  enum_source_file] + gdata_media_headers)

if r.returncode() != 0
  error('Couldn\'t generate source file - ' + enum_source_file)
endif

foreach header_file:gdata_media_private_headers
   gdata_private_headers += join_paths(gdata_media_path, header_file)
endforeach
