scrapbook_sources = ['scrapbook/scrapbook.c', 'scrapbook/scrapbook.h']

executable('scrapbook',
  scrapbook_sources[0],
  link_with: compiled_libs[0],
  c_args: demos_cargs,
  dependencies: gdata_deps + gdata_private_deps,
  include_directories: [top_inc_dir, gdata_inc_dir],
  sources: files(scrapbook_sources) +
           headers +
           gdata_private_headers +
           generated_enum_headers
)
