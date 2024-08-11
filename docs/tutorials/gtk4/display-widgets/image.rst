.. currentmodule:: gi.repository

Image
=====

:class:`Gtk.Image` is a widget to display images.
Various kinds of object can be displayed as an image.
Most typically, you would load a :class:`Gdk.Texture` from a file, using the
convenience function :meth:`Gtk.Image.new_from_file`, or
:meth:`Gtk.Image.new_from_icon_name`.

:class:`Gtk.Image` displays its image as an icon, with a size that is determined
by the application. See :doc:`/tutorials/gtk4/display-widgets/picture` if you want to show
an image at is actual size.

Sometimes an application will want to avoid depending on external data files,
such as image files.
See the documentation of :class:`Gio.Resource` inside GIO, for details.
In this case, :attr:`Gtk.Image.props.resource` and
:meth:`Gtk.Image.new_from_resource`, should be used.
