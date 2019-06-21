subdir('xml')

doc_module = 'gdata'

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
  # FIXME: theses files are created by enums workaround
  'gdata-documents-enums-in.h',
  'gdata-enums-in.h',
  'gdata-freebase-enums-in.h',
  'gdata-media-enums-in.h',
  'gdata-picasaweb-enums-in.h',
  'gdata-youtube-enums-in.h',
]

# FIXME: Currently, when you disable GOA support and try to build gtk-doc
# documentation, gtkdoc-mkhtml errors out because it tries to find those GOA
# symbols. To fix that, the particular sections in gdata-docs.xml and
# gdata-sections.txt have to be conditionally commented. It's not a major bug
# since at the only time generating documentation makes sense is at a release,
# and GOA needs toe be enabled.
if not enable_goa
  ignore_files += ['gdata-goa-authorizer.c', 'gdata-goa-authorizer.h']
endif

scan_args = [
  '--rebuild-types',
  '--deprecated-guards=LIBGDATA_DISABLE_DEPRECATED',
]

content_files = [
  'gdata-overview.xml',
  'gdata-running.xml',
]

# Images to copy into HTML directory.
html_assets = [
  'structure.png',
  'data-flow.png',
  'cancellation.png',
]

gnome.gtkdoc(
  doc_module,
  main_xml: doc_module + '-docs.xml',
  src_dir: gdata_inc_dir,
  ignore_headers: ignore_files,
  dependencies: libgdata_dep,
  namespace: doc_module,
  scan_args: scan_args,
  fixxref_args: '--html-dir=' + (gdata_prefix / gnome.gtkdoc_html_dir(gdata_name)),
  content_files: content_files,
  html_assets: html_assets,
  install: true,
)
