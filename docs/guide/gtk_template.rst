.. _guide-gtk-template:

============
Gtk.Template
============

A GtkWidget subclass can use a
`GtkBuilder UI Definition <https://docs.gtk.org/gtk4/class.Builder.html#gtkbuilder-ui-definitions>`__
XML document as a template to create child widgets and set its own
properties, without creating a GtkBuilder instance. This is implemented
for Python by PyGObject with Gtk.Template.

The subclass uses a ``@Gtk.Template`` decorator and declares a class
variable ``__gtype_name__`` with the value of the XML ``template``
element ``class`` attribute.

Child widgets are declared, typically with the same names as the XML
``object`` element ``id`` attributes, at the class level as instances
of ``Gtk.Template.Child``.

Signal handler methods, typically with the same names as the XML ``signal``
element ``handler`` attributes, use the ``@Gtk.Template.Callback`` decorator.

``Gtk.Template()`` takes a mandatory keyword argument passing the XML document
or its location, either ``string``, ``filename`` or ``resource_path``.

``Gtk.Template.Child()`` and ``Gtk.Template.Callback()`` optionally take
a ``name`` argument matching the value of the respective XML attribute,
in which case the Python attribute can have a different name.

Examples
--------

.. code-block:: python

    xml = """\
    <interface>
      <template class="example1" parent="GtkBox">
        <child>
          <object class="GtkButton" id="hello_button">
            <property name="label">Hello World</property>
            <signal name="clicked" handler="hello_button_clicked" swapped="no" />
          </object>
        </child>
      </template>
    </interface>
    """

    @Gtk.Template(string=xml)
    class Foo(Gtk.Box):
        __gtype_name__ = "example1"

        hello_button = Gtk.Template.Child()

        @Gtk.Template.Callback()
        def hello_button_clicked(self, *args):
            pass

Python attribute names that are different to the XML values:

.. code-block:: python

    @Gtk.Template(string=xml)
    class Foo(Gtk.Box):
        __gtype_name__ = "example1"

        my_button = Gtk.Template.Child("hello_button")

        @Gtk.Template.Callback("hello_button_clicked")
        def bar(self, *args):
            pass


Subclasses that declare ``__gtype_name__`` can be used as objects in the XML:

.. code-block:: python

    xml = """\
    <interface>
      <template class="example3" parent="GtkBox">
        <child>
          <object class="ExampleButton" id="hello_button">
            <property name="label">Hello World</property>
            <signal name="clicked" handler="hello_button_clicked" swapped="no" />
          </object>
        </child>
      </template>
    </interface>
    """


    class HelloButton(Gtk.Button):
        __gtype_name__ = "ExampleButton"


    @Gtk.Template(string=xml)
    class Foo(Gtk.Box):
        __gtype_name__ = "example3"

        hello_button = Gtk.Template.Child()

        @Gtk.Template.Callback()
        def hello_button_clicked(self, *args):
            pass
