ent_conf = {
  'PACKAGE': gdata_name,
  'PACKAGE_BUGREPORT': 'https://gitlab.gnome.org/GNOME/libgdata/issues',
  'PACKAGE_NAME': gdata_name,
  'PACKAGE_STRING': '@0@ @1@'.format(gdata_name, gdata_version),
  'PACKAGE_TARNAME': gdata_name,
  'PACKAGE_URL': 'https://wiki.gnome.org/Projects/libgdata',
  'PACKAGE_VERSION': gdata_version,
}

configure_file(
  input: 'gtkdocentities.ent.in',
  output: '@BASENAME@',
  configuration: ent_conf,
)
