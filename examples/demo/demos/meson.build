gnome = import('gnome')

python3 = import('python3')

appwindow_sources = [
  'appwindow.py',
]

install_data(appwindow_sources, install_dir: get_option('bindir'))

# desktop file
install_data('appwindow.desktop', install_dir: join_paths(get_option('datadir'), 'applications'))