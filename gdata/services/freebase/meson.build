gdata_freebase_path = join_paths('gdata', 'services', 'freebase')
gdata_freebase_include_path = join_paths(gdata_includedir, gdata_freebase_path)

gdata_freebase_headers = [
  'gdata-freebase-query.h',
  'gdata-freebase-result.h',
  'gdata-freebase-search-query.h',
  'gdata-freebase-search-result.h',
  'gdata-freebase-service.h',
  'gdata-freebase-topic-query.h',
  'gdata-freebase-topic-result.h',
]

gdata_freebase_enums = gnome.mkenums_simple('gdata-freebase-enums',
  sources : gdata_freebase_headers,
  install_header : true,
  install_dir : gdata_freebase_include_path)

install_headers(gdata_freebase_headers, install_dir: gdata_freebase_include_path)

# Add the enum header after install_header(), since enum headers are generated at compile time
gdata_freebase_headers += ['gdata-freebase-enums.h']
