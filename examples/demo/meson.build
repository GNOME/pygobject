if get_option('demos')
subdir('demos')

demo_app_sources = [
  'pygobject-demo.py',
]
# desktop file
install_data('org.gnome.pygobject-demo.desktop',
             install_dir: join_paths(get_option('datadir'), 'applications'))
install_data(demo_app_sources, install_dir: get_option('bindir'))

endif