gdata_atom_headers = files([
  'gdata-author.h',
  'gdata-category.h',
  'gdata-generator.h',
  'gdata-link.h'
])

install_headers(
  gdata_atom_headers,
  subdir: gdata_include_subdir / 'atom',
)

sources += files([
  'gdata-author.c',
  'gdata-category.c',
  'gdata-generator.c',
  'gdata-link.c'
])

headers += gdata_atom_headers
