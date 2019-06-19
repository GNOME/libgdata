demos_cargs = ['-DG_LOG_DOMAIN="libgdata"', '-DLIBGDATA_DISABLE_DEPRECATED']

# demos = [subdir, executable name, custom objects (for sources)]
demos = [
  ['calendar', 'calendar-cli', {'sources': 'calendar/calendar-cli.c'}],
  ['tasks', 'task-cli', {'sources': 'tasks/tasks-cli.c'}],
  ['youtube', 'youtube-cli', {'sources': 'youtube/youtube-cli.c'}],
]

if enable_goa.enabled()
  demos += [['docs-list', 'docs-list', {'sources': 'docs-list/docs-list.c'}]]
endif

if enable_gtk.enabled()
  demos += [['scrapbook', 'scrapbook', { 'sources': ['scrapbook/scrapbook.c', 'scrapbook/scrapbook.h']}]]
endif

foreach demo: demos
  executable(
    demo[1],
    link_with: libgdata_lib,
    c_args: demos_cargs,
    dependencies: [libgdata_dep] + gdata_deps + gdata_private_deps,
    include_directories: [top_inc_dir, gdata_inc_dir],
    kwargs: demo[2],
  )
endforeach
