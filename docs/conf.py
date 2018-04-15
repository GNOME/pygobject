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
    'cairo': ('https://pycairo.readthedocs.io/en/latest', None),
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
    "display_gitlab": True,
    "gitlab_user": "GNOME",
    "gitlab_repo": "pygobject",
    "gitlab_version": "master",
    "conf_py_path": "/docs/",
    "gitlab_host": "gitlab.gnome.org",
}

html_static_path = [
    "extra.css",
    "images/pygobject-small.svg",
]

html_theme_options = {
    "display_version": False,
}

extlinks = {
    'bzbug': ('https://bugzilla.gnome.org/show_bug.cgi?id=%s', 'bz#'),
    'issue': ('https://gitlab.gnome.org/GNOME/pygobject/issues/%s', '#'),
    'commit': ('https://gitlab.gnome.org/GNOME/pygobject/commit/%s', ''),
    'mr': (
        'https://gitlab.gnome.org/GNOME/pygobject/merge_requests/%s', '!'),
    'user': ('https://gitlab.gnome.org/%s', ''),
}

suppress_warnings = ["image.nonlocal_uri"]
