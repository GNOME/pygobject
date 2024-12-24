.. currentmodule:: gi.repository

Adwaita Application
===================

:class:`Adw.Application` extends :class:`Gtk.Application` to ease some task
related to creating applications for GNOME.

.. seealso::

    We previously touched :class:`Gtk.Application` in :doc:`this tutorial </tutorials/gtk4/application>`.


Adwaita also have :class:`Adw.ApplicationWindow`, it's a subclass of :class:`Gtk.ApplicationWindow`
that provides the same "freform" features from :class:`Adw.Window`.

.. note::
    Using :class:`Adw.Application` will also call :func:`Adw.init` for you, this
    function initialize de library, making sure that translations, types,
    themes, icons and stylesheets needed  by the library are set up properly.


Stylesheets
-----------

If you make use of :class:`Gio.Resource`, :class:`Adw.Application` will
automatically load stylesheets located in the application's resource base path.

This way you don't need to manually load stylesheets, and it will load the
matching stylesheets depending on the system appearance settings exposed by
:class:`Adw.StyleManager`.


* ``style.css`` contains the base styles.

* ``style-dark.css`` contains styles only used when :attr:`Adw.StyleManager.props.dark`
  is ``True``.

* ``style-hc.css`` contains styles used when :attr:`Adw.StyleManager.props.high_contrast` is ``True``.

* ``style-hc-dark.css`` contains styles used when :attr:`Adw.StyleManager.props.high_contrast`
  and :attr:`Adw.StyleManager.props.dark` are both ``True``.
