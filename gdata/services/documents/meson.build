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

enum_header_file = 'gdata-documents-enums.h'
enum_source_file = 'gdata-documents-enums.c'

# Generate the enum header file in the source code directory
r = run_command([
  join_paths(meson.source_root(), 'scripts/generate_enum_header.sh'),
  '-s', 's/g_data/gdata/',
  enum_header_file] + gdata_documents_headers)

if r.returncode() != 0
  error('Couldn\'t generate header file - ' + enum_header_file)
endif

gdata_documents_headers += [enum_header_file]
install_headers(files(gdata_documents_headers), install_dir: gdata_documents_include_path)

# Generate the enum source file in the source code directory
r = run_command([
  join_paths(meson.source_root(), 'scripts/generate_enum_source.sh'),
  '-s', 's/g_data/gdata/',
  enum_source_file] + gdata_documents_headers)

if r.returncode() != 0
  error('Couldn\'t generate source file - ' + enum_source_file)
endif

foreach header_file:gdata_documents_private_headers
   gdata_private_headers += join_paths(gdata_documents_path, header_file)
endforeach
