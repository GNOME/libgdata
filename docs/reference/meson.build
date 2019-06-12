if get_option('gtk_doc')
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
    '--deprecated-guards=G_DISABLE_DEPRECATED',
    '--ignore-decorators=' + '|'.join(ignore_files),
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

  # SGML files where gtk-doc abbrevations (#GtkWidget) are expanded
  # These files must be listed here *and* in content_files
  # e.g. expand_content_files=running.sgml
  expand_content_files = []

  docpath = join_paths(gdata_datadir, 'gtk-doc', 'html')
  gnome.gtkdoc('gdata',
               main_xml : 'gdata-docs.xml',
               namespace : 'gdata',
               mode : 'none',
               src_dir : ['gdata'],
               dependencies : compiled_libs[0],
               scan_args : gtkdoc_common_scan_args + [
                 '--ignore-headers=' + ' '.join(ignore_files),
               ],
               content_files : content_files,
               expand_content_files : expand_content_files,
               html_assets : html_assets,
               fixxref_args: [
                 '--html-dir=' + docpath,
               ],
               install: true)
endif
