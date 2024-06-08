System Dependencies
===================

PyGObject not only depends on packages available from PyPI, but also requires
certain system dependencies to be installed. Because their version isn't taken
into account by pip's dependency resolver, you may need to restrict the version
of PyGObject itself for the installation to succeed. This lists the minimum
version of the system dependencies over time.

3.50.0+:
    * glib: >= 2.80.0
    * libffi: >= 3.0
    * gobject-introspection: >= 1.64.0 (for tests only)

3.46.0+:
    * glib: >= 2.64.0
    * gobject-introspection: >= 1.64.0
    * libffi: >= 3.0

    Example distributions: Ubuntu 20.04, Debian Bullseye, or newer

3.40.0 - 3.44.x:
    * glib: >= 2.56.0
    * gobject-introspection: >= 1.56.0
    * libffi: >= 3.0

    Example distributions: Ubuntu 18.04, or newer
