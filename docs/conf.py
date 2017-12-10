# -*- coding: utf-8 -*-

extensions = [
    'sphinx.ext.todo',
    'sphinx.ext.intersphinx',
    'sphinx.ext.extlinks',
]

intersphinx_mapping = {
    'gtk': ('https://lazka.github.io/pgi-docs/Gtk-3.0', None),
    'gobject': ('https://lazka.github.io/pgi-docs/GObject-2.0', None),
    'glib': ('https://lazka.github.io/pgi-docs/GLib-2.0', None),
    'gdk': ('https://lazka.github.io/pgi-docs/Gdk-3.0', None),
    'gio': ('https://lazka.github.io/pgi-docs/Gio-2.0', None),
    'python2': ('https://docs.python.org/2.7', None),
    'python3': ('https://docs.python.org/3', None),
}

source_suffix = '.rst'
master_doc = 'index'
exclude_patterns = ['_build', 'README.rst']

pygments_style = 'tango'
html_theme = 'sphinx_rtd_theme'
html_show_copyright = False
html_favicon = "images/favicon.ico"
project = "PyGObject"
html_title = project

html_context = {
    'extra_css_files': [
        'https://quodlibet.github.io/fonts/font-mfizz.css',
        '_static/extra.css',
    ],
}

html_static_path = [
    "extra.css",
    "images/pygobject-small.svg",
]

html_theme_options = {
    "display_version": False,
}

extlinks = {
    'gnomebug': ('https://bugzilla.gnome.org/show_bug.cgi?id=%s', '#'),
}

suppress_warnings = ["image.nonlocal_uri"]
