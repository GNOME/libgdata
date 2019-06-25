headers = files(
  'gdata-calendar-access-rule.h',
  'gdata-calendar-calendar.h',
  'gdata-calendar-event.h',
  'gdata-calendar-feed.h',
  'gdata-calendar-query.h',
  'gdata-calendar-service.h',
)

install_headers(
  headers,
  subdir: gdata_include_subdir / 'services/calendar',
)

gir_headers += headers

sources += files(
  'gdata-calendar-access-rule.c',
  'gdata-calendar-calendar.c',
  'gdata-calendar-event.c',
  'gdata-calendar-feed.c',
  'gdata-calendar-query.c',
  'gdata-calendar-service.c',
)
