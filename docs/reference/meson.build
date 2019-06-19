ent_conf = configuration_data()

ent_conf.set_quoted('PACKAGE', gdata_name)
ent_conf.set_quoted('PACKAGE_NAME', gdata_name)
ent_conf.set_quoted('PACKAGE_STRING', '@0@ @1@'.format(gdata_name, gdata_version))
ent_conf.set_quoted('PACKAGE_VERSION', gdata_version)

configure_file(
  input: 'gtkdocentities.ent.in',
  output: 'gtkdocentities.ent',
  configuration: ent_conf,
)

ignore_files = [
  'common.h',
  'gdata-batch-feed.h',
  'gdata-batch-private.h',
  'gdata-buffer.h',
  'gdata-documents-enums.h',
  'gdata-documents-utils.h',
  'gdata-dummy-authorizer.c',
  'gdata-dummy-authorizer.h',
  'gdata-enums.h',
  'gdata-exif-tags.h',
  'gdata-gd-feed-link.h',
  'gdata-georss-where.h',
  'gdata-marshal.h',
  'gdata-media-enums.h',
  'gdata-media-group.h',
  'gdata-parser.h',
  'gdata-picasaweb-enums.h',
  'gdata-private.h',
  'gdata-youtube-enums.h',
  'mock-resolver.h',
  'mock-server.h',
]

gtkdoc_common_scan_args = [
  '--rebuild-types',
  '--deprecated-guards=LIBGDATA_DISABLE_DEPRECATED',
  '--ignore-headers=' + ' '.join(ignore_files),
]

# Images to copy into HTML directory.
html_assets = [
  'structure.png',
  'data-flow.png',
  'cancellation.png',
]

# Extra SGML files
content_files = [
  'gdata-overview.xml',
  'gdata-running.xml',
]

gnome.gtkdoc('gdata',
             main_xml: 'gdata-docs.xml',
             namespace: 'gdata',
             src_dir: meson.current_source_dir() / '..' / '..' / 'gdata',
             dependencies: libgdata_dep,
             scan_args: gtkdoc_common_scan_args,
             content_files: content_files,
             html_assets: html_assets,
             fixxref_args: '--html-dir=' + (gdata_prefix / gnome.gtkdoc_html_dir(gdata_name)),
             install: true)
