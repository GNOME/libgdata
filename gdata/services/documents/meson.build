incs += include_directories('.')

include_subdir = gdata_include_subdir / 'services/documents'

headers = files(
  'gdata-documents-access-rule.h',
  'gdata-documents-document.h',
  'gdata-documents-drawing.h',
  'gdata-documents-drive.h',
  'gdata-documents-drive-query.h',
  'gdata-documents-entry.h',
  'gdata-documents-feed.h',
  'gdata-documents-folder.h',
  'gdata-documents-metadata.h',
  'gdata-documents-pdf.h',
  'gdata-documents-presentation.h',
  'gdata-documents-property.h',
  'gdata-documents-query.h',
  'gdata-documents-service.h',
  'gdata-documents-spreadsheet.h',
  'gdata-documents-text.h',
  'gdata-documents-upload-query.h',
)

install_headers(
  headers,
  subdir: include_subdir,
)

gir_headers += headers

sources += files(
  'gdata-documents-access-rule.c',
  'gdata-documents-document.c',
  'gdata-documents-drawing.c',
  'gdata-documents-drive.c',
  'gdata-documents-drive-query.c',
  'gdata-documents-entry.c',
  'gdata-documents-feed.c',
  'gdata-documents-folder.c',
  'gdata-documents-metadata.c',
  'gdata-documents-pdf.c',
  'gdata-documents-presentation.c',
  'gdata-documents-property.c',
  'gdata-documents-query.c',
  'gdata-documents-service.c',
  'gdata-documents-spreadsheet.c',
  'gdata-documents-text.c',
  'gdata-documents-upload-query.c',
  'gdata-documents-utils.c',
)

enums = 'gdata-documents-enums'

# FIXME: Work around the namespace being incorrectly detected
# by glib-mkenums. This needs to be fixed by changing the
# namespace in libgdata. See !6.
enums_in = gnome.mkenums_simple(
  enums + '-in',
  sources: headers,
)

sources += custom_target(
  enums + '.c',
  input: enums_in[0],
  output: enums + '.c',
  command: enum_source_cmd,
  capture: true,
)

enum_headers += custom_target(
  enums + '.h',
  input: enums_in[1],
  output: enums + '.h',
  command: enum_header_cmd,
  capture: true,
  install: true,
  install_dir: gdata_includedir / include_subdir,
)
