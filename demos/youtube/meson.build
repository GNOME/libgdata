youtube_sources = ['youtube-cli.c']

executable('youtube-cli',
  youtube_sources[0],
  link_with: compiled_libs[0],
  c_args: demos_cargs,
  dependencies: gdata_deps + gdata_private_deps,
  include_directories: [top_inc_dir, gdata_inc_dir],
  sources: files(youtube_sources) +
           headers +
           gdata_private_headers +
           generated_enum_headers
)
