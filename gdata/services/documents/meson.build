gdata_documents_path = 'gdata/services/documents'
gdata_documents_include_path = join_paths(gdata_includedir, gdata_documents_path)

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
  files(gdata_documents_headers),
  install_dir: gdata_documents_include_path
)

enum_header_file = 'gdata-documents-enums.h'
enum_source_file = 'gdata-documents-enums.c'

# Generate the enum source file in the source code and build directories
gdata_documents_enum_header = custom_target(
  enum_header_file,
  command: [
    python_prog,
    join_paths(meson.current_source_dir(), '../../../scripts/generate_enum_header.py'),
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
    join_paths(meson.current_source_dir(), '../../../scripts/generate_enum_source.py'),
    enum_source_file,
    '--replace', 'g_data', 'gdata',
    '--source-dir', meson.current_source_dir(),
    '--out-dir', meson.current_build_dir(),
    '--sources'
    ] + gdata_documents_headers,
  output: [enum_source_file],
  console: true
)

# Copy all the header files required to compile the enum_source_file
r = run_command(
  cp_prog,
  ['-r'] + gdata_documents_headers + [meson.current_build_dir()],
  check: true
)

sources += files([
  'gdata-documents-service.c',
  'gdata-documents-metadata.c',
  'gdata-documents-feed.c',
  'gdata-documents-entry.c',
  'gdata-documents-document.c',
  'gdata-documents-text.c',
  'gdata-documents-presentation.c',
  'gdata-documents-spreadsheet.c',
  'gdata-documents-folder.c',
  'gdata-documents-drawing.c',
  'gdata-documents-pdf.c',
  'gdata-documents-query.c',
  'gdata-documents-upload-query.c',
  'gdata-documents-access-rule.c',
  'gdata-documents-utils.c'
])

headers += files(gdata_documents_headers)
generated_enum_headers += [gdata_documents_enum_header]
generated_enum_sources += [gdata_documents_enum_source]
