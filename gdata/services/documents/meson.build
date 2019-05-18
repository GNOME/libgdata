gdata_documents_path = 'gdata/services/documents'
gdata_documents_include_path = join_paths(gdata_includedir, gdata_documents_path)

gdata_documents_headers = [
  'gdata-documents-access-rule.h',
  'gdata-documents-document.h',
  'gdata-documents-drawing.h',
  'gdata-documents-entry.h',
  'gdata-documents-feed.h',
  'gdata-documents-folder.h',
  'gdata-documents-metadata.h',
  'gdata-documents-pdf.h',
  'gdata-documents-presentation.h',
  'gdata-documents-query.h',
  'gdata-documents-service.h',
  'gdata-documents-spreadsheet.h',
  'gdata-documents-text.h',
  'gdata-documents-upload-query.h',
]

gdata_documents_private_headers = [
  'gdata-documents-entry-private.h',
  'gdata-documents-utils.h',
]

gdata_documents_enums = gnome.mkenums_simple(
  'gdata-documents-enums',
  sources : gdata_documents_headers,
  install_header : true,
  install_dir : gdata_documents_include_path
)

install_headers(gdata_documents_headers, install_dir: gdata_documents_include_path)

# Add the enum header after install_header(), since enum headers are generated at compile time
gdata_documents_headers += ['gdata-documents-enums.h']

foreach header_file:gdata_documents_private_headers
   gdata_private_headers += join_paths(gdata_documents_path, header_file)
endforeach
