gdata_calendar_headers = files([
  'gdata-calendar-access-rule.h',
  'gdata-calendar-calendar.h',
  'gdata-calendar-event.h',
  'gdata-calendar-feed.h',
  'gdata-calendar-query.h',
  'gdata-calendar-service.h',
])

install_headers(
  gdata_calendar_headers,
  install_dir: join_paths(gdata_includedir, 'gdata/services/calendar')
)

sources += files([
  'gdata-calendar-access-rule.c',
  'gdata-calendar-calendar.c',
  'gdata-calendar-event.c',
  'gdata-calendar-feed.c',
  'gdata-calendar-query.c',
  'gdata-calendar-service.c',
])

headers += gdata_calendar_headers
