nordstjernen_gtk_sources = files(
  'appmain.c',
  'procview.c',
  'procwindow.c',
  '../rproc_http.c',
  '../ipc_http.c',
)

nordstjernen_gtk_deps = nordstjernen_deps + [gtk4_dep]
if host_machine.system() != 'windows'
  nordstjernen_gtk_deps += [cc.find_library('rt', required: false)]
endif

executable(
  'nordstjernen',
  nordstjernen_engine_sources + nordstjernen_app_sources +
    nordstjernen_gtk_sources +
    nordstjernen_win_resources + [polyfills_h],
  dependencies: nordstjernen_gtk_deps,
  c_args: ['-DNS_HAVE_GTK=1'],
  include_directories: include_directories('..', '.'),
  win_subsystem: nordstjernen_win_subsystem,
  link_args: nordstjernen_link_args,
  install: true,
)
