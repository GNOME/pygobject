extensions = [
    "sphinx.ext.todo",
    "sphinx.ext.intersphinx",
    "sphinx.ext.extlinks",
    "sphinx_copybutton",
]

import re
import shutil
from pathlib import Path


def post_process_build(app, exception):
    """Flatten static files and fix HTML paths for subdirectories."""
    if exception:
        return

    static = Path(app.outdir) / "_static"
    # Flatten images directory
    if (img := static / "images").exists():
        for f in img.iterdir():
            if f.is_file():
                shutil.move(str(f), str(static / f.name))
        img.rmdir()

    # Fix _static/ paths in HTML
    for html in Path(app.outdir).rglob("*.html"):
        depth = len(html.relative_to(app.outdir).parts) - 1
        prefix = "../" * depth + "_static/" if depth else "_static/"
        content = html.read_text(encoding="utf-8")
        if (new := re.sub(r'src="_static/', f'src="{prefix}', content)) != content:
            html.write_text(new, encoding="utf-8")


def setup(app):
    app.connect("build-finished", post_process_build)


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

html_static_path = ["images", "."]

extlinks = {
    "bzbug": ("https://bugzilla.gnome.org/show_bug.cgi?id=%s", "bz#%s"),
    "issue": ("https://gitlab.gnome.org/GNOME/pygobject/-/issues/%s", "#%s"),
    "commit": ("https://gitlab.gnome.org/GNOME/pygobject/commit/%s", "%s"),
    "mr": ("https://gitlab.gnome.org/GNOME/pygobject/-/merge_requests/%s", "!%s"),
    "user": ("https://gitlab.gnome.org/%s", "%s"),
    "devdocs": ("https://developer.gnome.org/documentation/%s.html", None),
}

suppress_warnings = ["image.nonlocal_uri"]
