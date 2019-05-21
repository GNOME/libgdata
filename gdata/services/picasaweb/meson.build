gdata_picasaweb_path = join_paths('gdata', 'services', 'picasaweb')

gdata_picasaweb_headers = [
  'gdata-picasaweb-service.h',
  'gdata-picasaweb-query.h',
  'gdata-picasaweb-file.h',
  'gdata-picasaweb-album.h',
  'gdata-picasaweb-user.h',
  'gdata-picasaweb-feed.h',
  'gdata-picasaweb-comment.h',
]

gdata_picasaweb_enums = gnome.mkenums('gdata-picasaweb-enums',
  sources : gdata_picasaweb_headers,
  h_template : 'gdata-picasaweb-enums.h.template',
  c_template : 'gdata-picasaweb-enums.c.template',
  install_header : true,
  install_dir : join_paths(includedir, 'services', 'picasaweb'))

gdata_picasaweb_headers += ['gdata-picasaweb-enums.h']

foreach headers_file:gdata_picasaweb_headers
   gdata_picasaweb_include_headers += join_paths(gdata_picasaweb_path, headers_file)
endforeach
