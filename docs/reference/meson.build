ignore_files = [
  'common.h',
  'mock-resolver.h',
  'mock-server.h',
  'gdata-private.h',
  'gdata-parser.h',
  'gdata-marshal.h',
  'gdata-enums.h',
  'gdata-media-enums.h',
  'gdata-media-group.h',
  'gdata-youtube-enums.h',
  'gdata-picasaweb-enums.h',
  'gdata-documents-enums.h',
  'gdata-exif-tags.h',
  'gdata-georss-where.h',
  'gdata-buffer.h',
  'gdata-batch-private.h',
  'gdata-batch-feed.h',
  'gdata-gd-feed-link.h',
  'gdata-dummy-authorizer.c',
  'gdata-dummy-authorizer.h',
  'gdata-documents-utils.h',
]

gtkdoc_common_scan_args = [
  '--rebuild-types',
  '--deprecated-guards=LIBGDATA_DISABLE_DEPRECATED'
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
             src_dir: gdata_inc_dir,
             dependencies: [libgdata_dep] + gdata_deps,
             scan_args: gtkdoc_common_scan_args + [
               '--ignore-headers=' + ' '.join(ignore_files),
             ],
             content_files: content_files,
             html_assets: html_assets,
             fixxref_args: '--html-dir=' + (gdata_prefix / gnome.gtkdoc_html_dir(gdata_name)),
             install: true)
