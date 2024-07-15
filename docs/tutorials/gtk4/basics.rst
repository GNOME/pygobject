.. currentmodule:: gi.repository

GTK4 Basics
===========

Main loop and Signals
---------------------
Like most GUI toolkits, GTK uses an event-driven programming model.
When the user is doing nothing, GTK+ sits in the main loop and waits for input.
If the user performs some action - say, a mouse click - then the main loop
"wakes up" and delivers an event to GTK.

When widgets receive an event, they frequently emit one or more signals.
Signals notify your program that "something interesting happened" by invoking
functions you've connected to the signal. Such functions are commonly known
as *callbacks*.
When your callbacks are invoked, you would typically take some action - for
example, when an Open button is clicked you might display a file chooser
dialog. After a callback finishes, GTK will return to the main loop and await
more user input.

:class:`Gtk.Application` will run the main loop for you, so you don't need to
worry about it.

A :class:`Gtk.Widget` it's also a :class:`GObject.Object`, so to know how to
interact with these signals you must read the
:ref:`GObject Basics <basics-signals>`.

.. seealso:: `Library initialization and main loop`_ in GTK documentation.

Properties
----------
Read: :ref:`GObject Basics: Properties <basics-properties>`.


.. _Library initialization and main loop: https://docs.gtk.org/gtk4/initialization.html
