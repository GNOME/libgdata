gdata_include_path = join_paths(gdata_includedir, 'gdata')

subdir('app')
subdir('atom')
# TODO: subdir('exif')
subdir('gcontact')
subdir('gd')
# TODO: subdir('georss')
subdir('media')
subdir('services')
# TODO: subdir('tests')

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
# TODO: Ask if these two have to be separately handled
  'gdata-types.h',
  'gdata-version.h',
]

if enable_goa.enabled()
  gdata_headers += ['gdata-goa-authorizer.h']
endif

gdata_enums = gnome.mkenums_simple(
  'gdata-enums',
  sources : gdata_headers,
  install_header : true,
  install_dir : gdata_include_path
)

install_headers(gdata_headers, install_dir: gdata_include_path)

# Add the enum header after install_header(), since enum headers are generated at compile time
gdata_headers += ['gdata-enums.h']

foreach header_file:gdata_private_headers
   gdata_private_headers += join_paths(gdata_path, header_file)
endforeach
