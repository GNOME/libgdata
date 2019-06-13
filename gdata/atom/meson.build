gdata_atom_headers = files([
  'gdata-author.h',
  'gdata-category.h',
  'gdata-generator.h',
  'gdata-link.h'
])

install_headers(
  gdata_atom_headers,
  install_dir: gdata_includedir / 'gdata/atom'
)

sources += files([
  'gdata-author.c',
  'gdata-category.c',
  'gdata-generator.c',
  'gdata-link.c'
])

headers += gdata_atom_headers
