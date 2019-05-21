gdata_documents_path = join_paths('gdata', 'services', 'documents')

gdata_documents_headers = [
  'gdata-documents-service.h',
  'gdata-documents-metadata.h',
  'gdata-documents-feed.h',
  'gdata-documents-entry.h',
  'gdata-documents-query.h',
  'gdata-documents-document.h',
  'gdata-documents-text.h',
  'gdata-documents-presentation.h',
  'gdata-documents-folder.h',
  'gdata-documents-drawing.h',
  'gdata-documents-pdf.h',
  'gdata-documents-spreadsheet.h',
  'gdata-documents-upload-query.h',
  'gdata-documents-access-rule.h',
]

gdata_documents_private_headers = [
  'gdata-documents-entry-private.h',
  'gdata-documents-utils.h',
]

gdata_documents_enums = gnome.mkenums('gdata-documents-enums',
  sources : gdata_documents_headers,
  h_template : 'gdata-documents-enums.h.template',
  c_template : 'gdata-documents-enums.c.template',
  install_header : true,
  install_dir : join_paths(includedir, 'services', 'documents'))

gdata_documents_headers += ['gdata-documents-enums.h']

foreach headers_file:gdata_documents_headers
   gdata_documents_include_headers += join_paths(gdata_documents_path, headers_file)
endforeach

foreach headers_file:gdata_documents_private_headers
   gdata_private_headers += join_paths(gdata_documents_path, headers_file)
endforeach
