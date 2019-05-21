gdata_youtube_path = join_paths('gdata', 'services', 'youtube')

gdata_youtube_headers = [
  'gdata-youtube-service.h',
  'gdata-youtube-video.h',
  'gdata-youtube-content.h',
  'gdata-youtube-credit.h',
  'gdata-youtube-query.h',
  'gdata-youtube-state.h',
  'gdata-youtube-category.h',
  'gdata-youtube-comment.h',
  'gdata-youtube-feed.h',
]

gdata_youtube_enums = gnome.mkenums('gdata-youtube-enums',
  sources : gdata_youtube_headers,
  h_template : 'gdata-youtube-enums.h.template',
  c_template : 'gdata-youtube-enums.c.template',
  install_header : true,
  install_dir : join_paths(includedir, 'services', 'youtube'))

gdata_youtube_headers += ['gdata-youtube-enums.h']

foreach headers_file:gdata_youtube_headers
   gdata_youtube_include_headers += join_paths(gdata_youtube_path, headers_file)
endforeach
