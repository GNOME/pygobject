extensions = [
    "sphinx.ext.todo",
    "sphinx.ext.intersphinx",
    "sphinx.ext.extlinks",
    "sphinx_copybutton",
]

intersphinx_mapping = {
    "python": ("https://docs.python.org/3", None),
    "cairo": ("https://pycairo.readthedocs.io/en/latest", None),
    "apidocs": (
        "https://api.pygobject.gnome.org/",
        None,
    ),
}

source_suffix = {".rst": "restructuredtext"}
master_doc = "index"
exclude_patterns = ["_build", "README.rst"]

pygments_style = "tango"
html_theme = "pydata_sphinx_theme"
html_show_copyright = False
html_favicon = "images/favicon.ico"
project = "PyGObject"
html_title = project

html_context = {
    "extra_css_files": [
        "_static/extra.css",
    ],
    "display_gitlab": True,
    "gitlab_user": "GNOME",
    "gitlab_repo": "pygobject",
    "gitlab_version": "main",
    "conf_py_path": "/docs/",
    "gitlab_host": "gitlab.gnome.org",
}

html_static_path = [
    "extra.css",
    "images/pygobject-small.svg",
    "images/arch.svg",
    "images/arch-dark.svg",
]

extlinks = {
    "bzbug": ("https://bugzilla.gnome.org/show_bug.cgi?id=%s", "bz#%s"),
    "issue": ("https://gitlab.gnome.org/GNOME/pygobject/-/issues/%s", "#%s"),
    "commit": ("https://gitlab.gnome.org/GNOME/pygobject/commit/%s", "%s"),
    "mr": ("https://gitlab.gnome.org/GNOME/pygobject/-/merge_requests/%s", "!%s"),
    "user": ("https://gitlab.gnome.org/%s", "%s"),
    "devdocs": ("https://developer.gnome.org/documentation/%s.html", None),
}

suppress_warnings = ["image.nonlocal_uri"]
