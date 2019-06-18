gdata_documents_inc_dir = include_directories('.')

include_subdir = gdata_include_subdir / 'services/documents'

gdata_private_headers += files([
  'gdata-documents-entry-private.h',
  'gdata-documents-utils.h',
])

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

install_headers(
  gdata_documents_headers,
  subdir: include_subdir,
)

enums = 'gdata-documents-enums'

# workaround to fix namespace
enums_in = gnome.mkenums_simple(
  enums + '-in',
  sources: gdata_documents_headers,
)

gdata_documents_enum_source = custom_target(
  enums + '.c',
  input: enums_in[0],
  output: enums + '.c',
  command: enum_source_cmd,
  capture: true,
)

gdata_documents_enum_header = custom_target(
  enums + '.h',
  input: enums_in[1],
  output: enums + '.h',
  command: enum_header_cmd,
  capture: true,
  install: true,
  install_dir: gdata_includedir / include_subdir,
)

sources += files([
  'gdata-documents-access-rule.c',
  'gdata-documents-document.c',
  'gdata-documents-drawing.c',
  'gdata-documents-entry.c',
  'gdata-documents-feed.c',
  'gdata-documents-folder.c',
  'gdata-documents-metadata.c',
  'gdata-documents-pdf.c',
  'gdata-documents-presentation.c',
  'gdata-documents-query.c',
  'gdata-documents-service.c',
  'gdata-documents-spreadsheet.c',
  'gdata-documents-text.c',
  'gdata-documents-upload-query.c',
  'gdata-documents-utils.c'
])

headers += files(gdata_documents_headers) + [gdata_documents_enum_header]
sources += [gdata_documents_enum_source]
