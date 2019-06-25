headers = files(
  'gdata-tasks-query.h',
  'gdata-tasks-service.h',
  'gdata-tasks-task.h',
  'gdata-tasks-tasklist.h',
)

install_headers(
  headers,
  subdir: gdata_include_subdir / 'services/tasks',
)

gir_headers += headers

sources += files(
  'gdata-tasks-query.c',
  'gdata-tasks-service.c',
  'gdata-tasks-task.c',
  'gdata-tasks-tasklist.c',
)
