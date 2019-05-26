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

enum_header_file = 'gdata-picasaweb-enums.h'
enum_source_file = 'gdata-picasaweb-enums.c'

# Generate the enum header file in the source code directory
r = run_command([
  join_paths(meson.source_root(), 'scripts/generate_enum_header.sh'),
  '-s', 's/g_data_picasa_web/gdata_picasaweb/',
  '-s', 's/GDATA_TYPE_DATA_PICASA_WEB/GDATA_TYPE_PICASAWEB/',
  enum_header_file] + gdata_picasaweb_headers)

if r.returncode() != 0
  error('Couldn\'t generate header file - ' + enum_header_file)
endif

gdata_picasaweb_headers += [enum_header_file]
install_headers(files(gdata_picasaweb_headers), install_dir: gdata_picasaweb_include_path)

# Generate the enum source file in the source code directory
r = run_command([
  join_paths(meson.source_root(), 'scripts/generate_enum_source.sh'),
  '-s', 's/g_data_picasa_web/gdata_picasaweb/',
  enum_source_file] + gdata_picasaweb_headers)

if r.returncode() != 0
  error('Couldn\'t generate source file - ' + enum_source_file)
endif
