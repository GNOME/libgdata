gdata_youtube_include_path = join_paths(gdata_includedir, 'gdata/services/youtube')

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

enum_header_file = 'gdata-youtube-enums.h'
enum_source_file = 'gdata-youtube-enums.c'

# Generate the enum header file in the source code directory
r = run_command([
  join_paths(meson.source_root(), 'scripts/generate_enum_header.sh'),
  '-s', 's/g_data_you_tube/gdata_youtube/',
  '-s', 's/GDATA_TYPE_DATA_YOU_TUBE/GDATA_TYPE_YOUTUBE/',
  enum_header_file] + gdata_youtube_headers)

if r.returncode() != 0
  error('Couldn\'t generate header file - ' + enum_header_file)
endif

gdata_youtube_headers += [enum_header_file]
install_headers(files(gdata_youtube_headers), install_dir: gdata_youtube_include_path)

# Generate the enum source file in the source code directory
r = run_command([
  join_paths(meson.source_root(), 'scripts/generate_enum_source.sh'),
  '-s', 's/g_data_you_tube/gdata_youtube/',
  enum_source_file] + gdata_youtube_headers)

if r.returncode() != 0
  error('Couldn\'t generate source file - ' + enum_source_file)
endif

# Add the enum header after install_header(), since enum headers are generated at compile time
gdata_youtube_headers += ['gdata-youtube-enums.h']
