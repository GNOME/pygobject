============================
Porting from Static Bindings
============================

Before PyGObject 3, bindings where not generated automatically through gobject
introspection and where provided as separate Python libraries like pygobject,
pygtk, pygst etc. We call them static bindings.

If your code contains imports like ``import gtk``, ``import gst``, ``import
glib`` or ``import gobject`` you are using the old bindings and you should
upgrade.

Note that using old and new bindings in the same process is not supported, you
have to switch everything at once.


Static Bindings Library Differences
-----------------------------------

**pygtk** supported GTK 2.0 and Python 2 only. PyGObject supports GTK >=3.0
and Python 2/3. If you port away from pygtk you also have to move to GTK 3.0
at the same time. **pygtkcompat** described below can help you with that
transition.

**pygst** supports GStreamer 0.10 and Python 2 only. Like with GTK you have
to move to PyGObject and GStreamer 1.0 at the same time.

**pygobject 2** supports glib 2.0 and Python 2. The new bindings also support
glib 2.0 and Python 2/3.

.. note::
    Python 2 is no longer supported since PyGObject 3.38.0.

General Porting Tips
--------------------

PyGObject contains a shell script which can help you with the many naming
differences between static and dynamic bindings:

https://gitlab.gnome.org/GNOME/pygobject/raw/master/tools/pygi-convert.sh

::

    ./pygi-convert.sh mymodule.py

It just does basic text replacement. It reduces the amount of naming changes
you have to make in the beginning, but nothing more.

1) Run on a Python module
2) Check/Verify the changes made (e.g. using ``git diff``)
3) Finish porting the module by hand
4) Continue to the next module...


Porting Tips for GTK
--------------------

PyGObject does not support GTK 2.0. In order to use PyGObject, you'll need
to port your code to GTK 3.0 right away.

For some general advice regarding the migration from GTK 2.0 to 3.0 see the
`offical migration guide
<https://docs.gtk.org/gtk3/migrating-2to3.html>`__. If you
need to know how a C symbol is exposed in Python have a look at the `symbol
mapping listing <https://lazka.github.io/pgi-docs/#Gtk-3.0/mapping.html>`__.


Using the pygtkcompat Compatibility Layer
^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^^

.. note

   The pygtkcompat module is deprecated since PyGObject 3.46.
   If your code is dependent on pygtkcompat, you have two options:

   1. Update your code to use the GTK interface directly
   2. Copy the bits you need into your own application

    As of PyGObject 3.48, the compatibility layer will be disfunctional,
    and it will be completely removed in 3.50.

PyGObject versions prior to 3.48 ship a compatibility layer for pygtk which partially emulates the
old interfaces:

::

    from gi import pygtkcompat
    pygtkcompat.enable()
    pygtkcompat.enable_gtk(version='3.0')

    import gtk

``enable()`` has to be called once before the first ``gtk`` import.

Note that pygtkcompat is just for helping you through the transition by
allowing you to port one module at a time. Only a limited subset of the
interfaces are emulated correctly and you should try to get rid of it in the
end.


Default Encoding Changes
^^^^^^^^^^^^^^^^^^^^^^^^

Importing ``gtk`` had the side effect of changing the default Python encoding
from ASCII to UTF-8 (check ``sys.getdefaultencoding()``) and that no longer
happens with PyGObject. Since text with pygtk is returned as utf-8 encoded
str, your code is likely depending auto-decoding in many places and you can
change it manually by doing:

::

    # Python 2 only
    import sys
    reload(sys)
    sys.setdefaultencoding("utf-8")
    # see if auto decoding works:
    assert '\xc3\xb6' + u'' ==  u'\xf6'

While this is not officially supported by Python I don't know of any
downsides. Once you are sure that you explicitly decode in all places or you
move to Python 3 where things are unicode by default you can remove this
again.
