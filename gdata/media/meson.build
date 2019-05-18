gdata_media_path = join_paths('gdata', 'media')
gdata_media_include_path = join_paths(gdata_includedir, gdata_media_path)

gdata_media_headers = [
	'gdata-media-category.h',
	'gdata-media-content.h',
	'gdata-media-credit.h',
	'gdata-media-thumbnail.h',
]

gdata_media_private_headers = ['gdata-media-group.h']

gdata_media_enums = gnome.mkenums_simple('gdata-media-enums',
  sources : gdata_media_headers,
  install_header : true,
  install_dir : gdata_media_include_path)

install_headers(gdata_media_headers, install_dir: gdata_media_include_path)

# Add the enum header after install_header(), since enum headers are generated at compile time
gdata_media_headers += ['gdata-media-enums.h']

foreach header_file:gdata_media_private_headers
   gdata_private_headers += join_paths(gdata_media_path, header_file)
endforeach
