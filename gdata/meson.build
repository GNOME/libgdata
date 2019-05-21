gdata_path = 'gdata'

subdir('app')
# subdir('atom')
# subdir('exif')
# subdir('gcontact')
# subdir('gd')
# subdir('georss')
subdir('media')
subdir('services')
# subdir('tests')

gdata_marshal_files = gnome.genmarshal('gdata-marshal',
  sources: 'gdata-marshal.list')

gdata_private_headers += [
  'gdata-private.h',
  'gdata-batch-private.h',
  'gdata-batch-feed.h',
  'gdata-parser.h',
  'gdata-buffer.h',
  'gd/gdata-gd-feed-link.h',
  'exif/gdata-exif-tags.h',
  'georss/gdata-georss-where.h',
]

gdata_headers = [
  'gdata.h',
  'gdata-entry.h',
  'gdata-feed.h',
  'gdata-service.h',
  'gdata-query.h',
  'gdata-commentable.h',
  'gdata-comment.h',
  'gdata-access-handler.h',
  'gdata-access-rule.h',
  'gdata-parsable.h',
  'gdata-download-stream.h',
  'gdata-upload-stream.h',
  'gdata-comparable.h',
  'gdata-batch-operation.h',
  'gdata-batchable.h',
  'gdata-authorizer.h',
  'gdata-authorization-domain.h',
  'gdata-client-login-authorizer.h',
  'gdata-oauth1-authorizer.h',
  'gdata-oauth2-authorizer.h',
]

if enable_goa
  gdata_headers += ['gdata-goa-authorizer.h']
endif

gdata_enums = gnome.mkenums('gdata-enums',
  sources : gdata_headers,
  h_template : 'gdata-enums.h.template',
  c_template : 'gdata-enums.c.template',
  install_header : true,
  install_dir : includedir)

gdata_headers += [
  'gdata-enums.h',
  'gdata-types.h',
  'gdata-version.h',
]

foreach headers_file:gdata_headers
   gdata_include_headers += join_paths(gdata_path, headers_file)
endforeach

foreach headers_file:gdata_private_headers
   gdata_private_headers += join_paths(gdata_path, headers_file)
endforeach
