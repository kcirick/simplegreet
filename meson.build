project (
  'simplegreet',
  'c',
  version: '0.1',
  license: 'MIT',
  meson_version: '>=0.47.0',
  default_options: [
  'c_std=c11',
  'warning_level=2',
  ],
)

prefix = get_option('prefix')

dependencies = [dependency('gtk+-3.0'), dependency('json-c')]

gtk_layer_shell = dependency('gtk-layer-shell-0', required: get_option('layershell'))
if gtk_layer_shell.found()
  dependencies += gtk_layer_shell
  add_project_arguments('-DLAYER_SHELL', language : 'c')
endif

executable (
  'simplegreet',
  [ 'main.c',
    'proto.c',
    'actions.c',
    'window.c',
    'greeter.c'
  ],
  dependencies: dependencies,
  install: true
)
