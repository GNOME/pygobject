.. currentmodule:: gi.repository

==============
Error Handling
==============

GLib has its own method of handling errors using :obj:`GLib.Error`. These are
raised as Python exceptions, but with a few small differences.

It's common in Python for exception subclasses to be used (e.g.,
:obj:`ValueError` versus :obj:`IOError`) to distinguish different types of
errors. Libraries often define their own :obj:`Exception` subclasses, and
library users will handle these cases explicitly.

In GLib-using libraries, errors are all :obj:`GLib.Error` instances, with no
subclassing for different error types. Instead, every :obj:`GLib.Error`
instance has attributes that distinguish types of error:

* :attr:`GLib.Error.domain` is the error domain, usually a string that you can
  convert to a ``GLib`` quark with :func:`GLib.quark_from_string`
* :attr:`GLib.Error.code` identifies a specific error within the domain
* :attr:`GLib.Error.message` is a human-readable description of the error

Error domains are defined per-module, and you can get an error domain from
``*_error_quark`` functions on the relevant module. For example, IO errors
from ``Gio`` are in the domain returned by :func:`Gio.io_error_quark`, and
possible error code values are enumerated in :obj:`Gio.IOErrorEnum`.

Once you've caught a :obj:`GLib.Error`, you can call
:meth:`GLib.Error.matches` to see whether it matches the specific error you
want to handle.


Examples
--------

Catching a specific error:

.. code:: pycon

    >>> from gi.repository import GLib, Gio
    >>> f = Gio.File.new_for_path('missing-path')
    >>> try:
    ...     f.read()
    ... except GLib.Error as err:
    ...     if err.matches(Gio.io_error_quark(), Gio.IOErrorEnum.NOT_FOUND):
    ...         print('File not found')
    ...     else:
    ...         raise
    File not found
