demos_cargs = ['-DG_LOG_DOMAIN="libgdata"', '-DLIBGDATA_DISABLE_DEPRECATED']

subdir('calendar')
subdir('youtube')
subdir('tasks')

if enable_goa.enabled()
  subdir('docs-list')
endif

if enable_gtk.enabled()
  subdir('scrapbook')
endif
