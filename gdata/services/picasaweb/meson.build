gdata_picasaweb_path = join_paths('gdata', 'services', 'picasaweb')
gdata_picasaweb_include_path = join_paths(gdata_includedir, gdata_picasaweb_path)

gdata_picasaweb_headers = [
  'gdata-picasaweb-album.h',
  'gdata-picasaweb-comment.h',
  'gdata-picasaweb-feed.h',
  'gdata-picasaweb-file.h',
  'gdata-picasaweb-query.h',
  'gdata-picasaweb-service.h',
  'gdata-picasaweb-user.h',
]

gdata_picasaweb_enums = gnome.mkenums_simple('gdata-picasaweb-enums',
  sources : gdata_picasaweb_headers,
  install_header : true,
  install_dir : gdata_picasaweb_include_path)

install_headers(gdata_picasaweb_headers, install_dir: gdata_picasaweb_include_path)

# Add the enum header after install_header(), since enum headers are generated at compile time
gdata_picasaweb_headers += ['gdata-picasaweb-enums.h']
