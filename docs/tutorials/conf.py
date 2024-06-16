# Configuration file for the Sphinx documentation builder.
#
# This file only contains a selection of the most common options. For a full
# list see the documentation:
# https://www.sphinx-doc.org/en/master/usage/configuration.html

# -- Path setup --------------------------------------------------------------

# If extensions (or modules to document with autodoc) are in another directory,
# add these directories to sys.path here. If the directory is relative to the
# documentation root, use os.path.abspath to make it absolute, like shown here.
#
# import os
# import sys
# sys.path.insert(0, os.path.abspath('.'))


# -- Project information -----------------------------------------------------

project = 'PyGObject Guide'

# -- General configuration ---------------------------------------------------

# Add any Sphinx extension module names here, as strings. They can be
# extensions coming with Sphinx (named 'sphinx.ext.*') or your custom
# ones.
extensions = [
    'sphinx.ext.intersphinx',
    'sphinx.ext.extlinks',
    'sphinx.ext.autosectionlabel',
    'sphinx_copybutton',
]

intersphinx_mapping = {
    'apidocs': (
        'https://amolenaar.pages.gitlab.gnome.org/pygobject-docs',
        None,
    ),
    'pygobject': ('https://pygobject.gnome.org/', None),
    'python': ('https://docs.python.org/3', None),
}

extlinks = {
    'devdocs': ('https://developer.gnome.org/documentation/%s.html', None)
}

autosectionlabel_prefix_document = True

# Add any paths that contain templates here, relative to this directory.
templates_path = ['_templates']

# List of patterns, relative to source directory, that match files and
# directories to ignore when looking for source files.
# This pattern also affects html_static_path and html_extra_path.
exclude_patterns = []

# -- Options for HTML output -------------------------------------------------

# The theme to use for HTML and HTML Help pages.  See the documentation for
# a list of builtin themes.
#
html_theme = 'furo'
html_title = project
html_css_files = ['custom.css']

# Add any paths that contain custom static files (such as style sheets) here,
# relative to this directory. They are copied after the builtin static files,
# so a file named 'default.css' will overwrite the builtin 'default.css'.
html_static_path = ['_static']
html_show_copyright = 0
html_show_sphinx = 0
show_source = 0

html_theme_options = {
    'announcement': 'This site is under heavy development and some parts are unfinished.',
    'source_edit_link': 'https://gitlab.gnome.org/rafaelmardojai/pygobject-guide/-/blob/main/source/{filename}',
}
