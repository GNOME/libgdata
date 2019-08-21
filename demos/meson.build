demos_c_args = [
  '-DLIBGDATA_DISABLE_DEPRECATED',
]

# demos = [subdir, executable name, custom objects (for sources)]
demos = [
  ['calendar', 'calendar-cli', {'sources': 'calendar/calendar-cli.c'}],
  ['tasks', 'task-cli', {'sources': 'tasks/tasks-cli.c'}],
  ['youtube', 'youtube-cli', {'sources': 'youtube/youtube-cli.c'}],
]

if enable_goa
  demos += [
    ['docs-list', 'docs-list', {'sources': 'docs-list/docs-list.c'}],
    ['docs-property', 'docs-property', {'sources': 'docs-property/docs-property.c'}]
  ]
endif

if gtk_dep.found()
  demos += [['scrapbook', 'scrapbook', {'sources': 'scrapbook/scrapbook.c', 'dependencies': [gtk_dep]}]]
endif

foreach demo: demos
  kwargs = demo[2] + {'dependencies': [libgdata_dep] + demo[2].get('dependencies', [])}

  executable(
    demo[1],
    c_args: demos_c_args + common_c_args + ['-DG_LOG_DOMAIN="@0@"'.format(demo[1])],
    include_directories: top_inc_dir,
    kwargs: kwargs,
  )
endforeach
