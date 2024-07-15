.. currentmodule:: gi.repository

Spinner
=======
The :class:`Gtk.Spinner` displays an icon-size spinning animation.
It is often used as an alternative to a :class:`Gtk.ProgressBar`
for displaying indefinite activity, instead of actual progress.

To start the animation, use :meth:`Gtk.Spinner.start`,
to stop it use :meth:`Gtk.Spinner.stop`.

Example
-------

.. image:: images/spinner.png

.. literalinclude:: examples/spinner.py
    :linenos:


Extended example
----------------
An extended example that uses a timeout function to start and stop
the spinning animation.
The :func:`on_timeout` function is called at regular intervals
until it returns ``False``, at which point the timeout is automatically
destroyed and the function will not be called again.

Example
^^^^^^^

.. image:: images/spinner_ext.png

.. literalinclude:: examples/spinner_ext.py
    :linenos:
