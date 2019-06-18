gdata_documents_subdir = 'gdata/services/documents'
gdata_documents_include_path = gdata_includedir / gdata_documents_subdir

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
  install_dir: gdata_documents_include_path
)

enum_header_file = 'gdata-documents-enums.h'
enum_source_file = 'gdata-documents-enums.c'

# Generate the enum source file in the source code and build directories
gdata_documents_enum_header = custom_target(
  enum_header_file,
  command: [
    python_prog,
    enum_header_generator_script,
    enum_header_file,
    '--replace', 'g_data', 'gdata',
    '--source-dir', meson.current_source_dir(),
    '--out-dir', meson.current_build_dir(),
    '--sources'
    ] + gdata_documents_headers,
  install: true,
  install_dir: gdata_documents_include_path,
  output: [enum_header_file],
  console: true
)

# Generate the enum source file in the source code directory
gdata_documents_enum_source = custom_target(
  enum_source_file,
  command: [
    python_prog,
    enum_source_generator_script,
    enum_source_file,
    '--replace', 'g_data', 'gdata',
    '--source-dir', meson.current_source_dir(),
    '--out-dir', meson.current_build_dir(),
    '--include-dir', gdata_documents_subdir,
    '--sources'
    ] + gdata_documents_headers,
  output: [enum_source_file],
  console: true
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
