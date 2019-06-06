gdata_tasks_headers = files([
  'gdata-tasks-query.h',
  'gdata-tasks-service.h',
  'gdata-tasks-task.h',
  'gdata-tasks-tasklist.h',
])

install_headers(
  gdata_tasks_headers,
  install_dir: join_paths(gdata_includedir, 'gdata/services/tasks')
)

sources += files([
  'gdata-tasks-service.c',
  'gdata-tasks-tasklist.c',
  'gdata-tasks-task.c',
  'gdata-tasks-query.c'
])

headers += gdata_tasks_headers
