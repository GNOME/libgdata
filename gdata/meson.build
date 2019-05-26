gdata_path = 'gdata'
gdata_include_path = join_paths(gdata_includedir, gdata_path)

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
]

# TODO: Handle these separately later
# 'gdata-types.h',
# 'gdata-version.h'

if enable_goa.enabled()
  gdata_headers += ['gdata-goa-authorizer.h']
endif

enum_header_file = 'gdata-enums.h'
enum_source_file = 'gdata-enums.c'

# Generate the enum header file in the source code directory
r = run_command([
  join_paths(meson.source_root(), 'scripts/generate_enum_header.sh'),
  '-s', 's/g_data/gdata/',
  enum_header_file] + gdata_headers)

if r.returncode() != 0
  error('Couldn\'t generate header file - ' + enum_header_file)
endif

gdata_headers += [enum_header_file]
install_headers(files(gdata_headers), install_dir: gdata_include_path)

# Generate the enum source file in the source code directory
r = run_command([
  join_paths(meson.source_root(), 'scripts/generate_enum_source.sh'),
  '-s', 's/g_data/gdata/',
  enum_source_file] + gdata_headers)

if r.returncode() != 0
  error('Couldn\'t generate source file - ' + enum_header_file)
endif

foreach header_file:gdata_private_headers
   gdata_private_headers += join_paths(gdata_path, header_file)
endforeach
