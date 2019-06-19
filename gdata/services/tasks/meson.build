gdata_tasks_headers = files(
  'gdata-tasks-query.h',
  'gdata-tasks-service.h',
  'gdata-tasks-task.h',
  'gdata-tasks-tasklist.h',
)

install_headers(
  gdata_tasks_headers,
  subdir: gdata_include_subdir / 'services/tasks',
)

sources += files(
  'gdata-tasks-query.c',
  'gdata-tasks-service.c',
  'gdata-tasks-task.c',
  'gdata-tasks-tasklist.c',
)

headers += gdata_tasks_headers
