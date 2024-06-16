.. _guide-api:

================
GI Documentation
================

This is the API provided by the toplevel "gi" package.


.. function:: gi.require_version(namespace, version)

    :param str namespace: The namespace
    :param str version: The version of the namespace which should be loaded
    :raises: ..py:exception:: ValueError

    Ensures the namespace gets loaded with the given version. If the namespace
    was already loaded with a different version or a different version was
    required previously raises ValueError.

    ::

        import gi
        gi.require_version('Gtk', '3.0')


.. function:: gi.require_foreign(namespace, symbol=None)

    :param str namespace:
        Introspection namespace of the foreign module (e.g. "cairo")
    :param symbol:
        Optional symbol typename to ensure a converter exists.
    :type symbol: :obj:`str` or :obj:`None`
    :raises: ..py:exception:: ImportError

    Ensure the given foreign marshaling module is available and loaded.

    Example:

    .. code-block:: python

        import gi
        import cairo
        gi.require_foreign('cairo')
        gi.require_foreign('cairo', 'Surface')


.. function:: gi.check_version(version)

    :param tuple version: A version tuple
    :raises: ..py:exception:: ValueError

    Compares the passed in version tuple with the gi version and does nothing
    if gi version is the same or newer. Otherwise raises ValueError.


.. function:: gi.get_required_version(namespace)

    :returns: The version successfully required previously by :func:`gi.require_version` or :obj:`None`
    :rtype: str or :obj:`None`


.. data:: gi.version_info
    :annotation: = (3, 18, 1)

    The version of PyGObject


.. class:: gi.PyGIDeprecationWarning

    The warning class used for deprecations in PyGObject and the included
    Python overrides. It inherits from DeprecationWarning and is hidden
    by default.


.. class:: gi.PyGIWarning

    Like :class:`gi.PyGIDeprecationWarning` but visible by default.
