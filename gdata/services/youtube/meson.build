gdata_youtube_path = join_paths('gdata', 'services', 'youtube')
gdata_youtube_include_path = join_paths(gdata_includedir, gdata_youtube_path)

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

gdata_youtube_enums = gnome.mkenums_simple('gdata-youtube-enums',
  sources : gdata_youtube_headers,
  install_header : true,
  install_dir : gdata_youtube_include_path)

install_headers(gdata_youtube_headers, install_dir: gdata_youtube_include_path)

# Add the enum header after install_header(), since enum headers are generated at compile time
gdata_youtube_headers += ['gdata-youtube-enums.h']
