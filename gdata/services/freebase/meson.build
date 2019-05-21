gdata_freebase_path = join_paths('gdata', 'services', 'freebase')

gdata_freebase_headers = [
  'gdata-freebase-service.h',
  'gdata-freebase-result.h',
  'gdata-freebase-search-query.h',
  'gdata-freebase-search-result.h',
  'gdata-freebase-topic-query.h',
  'gdata-freebase-topic-result.h',
  'gdata-freebase-query.h',
]

gdata_freebase_enums = gnome.mkenums('gdata-freebase-enums',
  sources : gdata_freebase_headers,
  h_template : 'gdata-freebase-enums.h.template',
  c_template : 'gdata-freebase-enums.c.template',
  install_header : true,
  install_dir : join_paths(includedir, 'services', 'freebase'))

gdata_freebase_headers += ['gdata-freebase-enums.h']

foreach headers_file:gdata_freebase_headers
   gdata_freebase_include_headers += join_paths(gdata_freebase_path, headers_file)
endforeach
