Object Reference Counting for VFuncs and Closures
==================================================

Abstract
--------

The following document attempts to analyse marshaling strategies for GObjects passed as in or out
arguments to Python implemented vfuncs and signal closures. There are currently a number of bugs
which have cropped up throughout the years regarding this topic. Fixing these bugs sometimes ends
up causing problems in other areas of the bindings, or resulting in different leaks or invalid
unrefs to occur. Given this, analysis is needed which attempts to take all possibilities into
account. This can then be used as a basis for testing and implementation.

Related Bugs
~~~~~~~~~~~~

=======  ============================================================================
Bug      Description
=======  ============================================================================
638267_  gc.collect deletes __dict__ of an object in a GTK-object, Python-object cycle
675726_  props accessor leaks references for properties of type G_TYPE_OBJECT
687522_  Setting tool item type fails
657202_  Floating refs and transfer-ownership
546802_  Pygtk destroys cycle too early
640868_  Wrong gobject refcount when calling introspected constructors for widgets
661359_  Something is wrong with editing-started signal
92955_   gc.collect() destroys __dict__?
685598_  Callback closures and user_data with "call" scope type are leaked
=======  ============================================================================

.. _638267: http://bugzilla.gnome.org/show_bug.cgi?id=638267
.. _675726: http://bugzilla.gnome.org/show_bug.cgi?id=675726
.. _687522: http://bugzilla.gnome.org/show_bug.cgi?id=687522
.. _657202: http://bugzilla.gnome.org/show_bug.cgi?id=657202
.. _546802: http://bugzilla.gnome.org/show_bug.cgi?id=546802
.. _640868: http://bugzilla.gnome.org/show_bug.cgi?id=640868
.. _661359: http://bugzilla.gnome.org/show_bug.cgi?id=661359
.. _92955: http://bugzilla.gnome.org/show_bug.cgi?id=92955
.. _685598: http://bugzilla.gnome.org/show_bug.cgi?id=685598

Assumptions
-----------

Possibilities that probably don't make sense:

* Inout arguments for a GObject should be considered invalid. Because GObject arguments are
  references to a managed object, an inout argument is ambiguous and should be a gi scanner error.

* Transfer container seems ambiguous in regards to GObject arguments and should probably be a gi
  scanner error.

Input Arguments
---------------

Input Argument with Transfer Everything
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

In this case we simply need to avoid adding an additional ref to the GObject argument and let the
bindings wrappers like PyObject steal an existing ref from it. However, if an input argument is
marked as transfer everything and the incoming object is floating, we should probably sink the
floating ref.

Input Argument with Transfer Nothing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is mostly a case of adding a new ref to the GObject which is removed upon the PyObject wrapper
being destructed. However, there are cases where incoming GObjects are floating which makes things
more complex. For example (from bug: 687522_)

.. code-block:: python

    class Window(Gtk.Window):
        def __init__(self):
            Gtk.Window.__init__(self)
            renderer = Gtk.CellRendererText()
            renderer.connect('editing-started', self.on_view_label_cell_editing_started)

        def on_view_label_cell_editing_started(self, renderer, editable, path):
            print path

In the above example "editable" is a floating reference which the bindings will currently sink and
add a reference to which ends up leaking. This could be addressed by maintaining the GObject as
floating and simply adding a new reference upon (in) argument marshaling as proposed in bug
675726_. However, in addition to this proposed fix, there is also a situation where we might
consider sinking the GObject within cleanup code after the Python vfunc is finished:

.. code-block:: python

    class Window(Gtk.Window):
        def on_view_label_cell_editing_started(self, renderer, editable, path):
            self.editable = editable

In this situation, Python is keeping a strong reference to what was just a temporary floating object
coming in. We could detect this case by looking at the PyObject ref count and if it is greater than
one, we sink the GObject because Python is basically maintaining a strong reference.

Output and Return Arguments
---------------------------

Output and return arguments should essentially be considered the same and behave the same in regards
to reference counting between callers and callees. The only difference is the code paths will be
different in the binding layer due to different APIs in gi-repository for working with them.

VFunc Return with Transfer Everything
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is perhaps the easiest case to deal with because bindings can simply increment the reference
count of the GObject being returned and return it (a "new value reference" in Python C API
terminology). Binding wrapper objects will decrement their existing reference to the GObject when
their own reference count reaches zero. This will always work regardless of if the object is only
temporarily created in Python and returned right back to C, or if the vfunc is returning a strong
reference to an object being held elsewhere.

VFunc Return with Transfer Nothing
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

This is one of the more problematic situations due to there not being a clear indication that
callers of the vfunc expect a floating reference or a borrowed reference. In Gtk for example, we
might have something like this:

.. code-block:: python

    class ToolMenuAction(Gtk.Action):
        def do_create_tool_item(self):
            return Gtk.MenuToolButton()

The problem here is the creation MenuToolButton from Python will sink the initial floating
reference. However, what the caller of create_tool_item really wants you to return is a floating
reference. Currently the Python bindings are returning an invalid object do to the lifetime being
created and destroyed all within the scope of the Python method (bug 687522_). If the bindings
were to always ref return values (just like with transfer everything), it would most likely leak a
reference as it would be returning a strong reference which is not owned by Python and the caller
expects a floating reference. There are a number of potential solutions:

1. Don't sink floating references upon object creation, but add a new reference. This would fix the
   immediate problem but more thought needs to be put into potential fallout. Another thing to
   consider is if the callee holds onto the reference it returns, we would want to sink the ref
   before returning the GObject. This can be detected in marshaling code by looking at the ref
   count of the PyObject wrapper, if it is greater than one and the GObject is floating, we sink
   it. This would then essentially be returning a "borrowed reference" and in this case the caller
   would most likely be using g_object_ref_sink which would add it's own new reference.

2. A sort of opposite idea from the first one would be to always sink floating refs as we do now,
   but re-float and add an extra ref upon out argument marshaling if the object is derived from
   GInitallyUnowned and Python would otherwise destroy it (the PyObject and GObject ref counts are
   only one). Likewise, if the PyObject ref count is greater than one, we are returning a borrowed
   reference as expected and would not want to re-float the GObject.

3. Always require vfuncs to use transfer full (a new value reference), this would basically solve
   all the problems but is unrealistic. The reason this is nice is there is really no confusion
   about floating refs or methods which imply returning an internal borrowed reference (something
   like Atk.Object.get_parent)

Returning Borrowed References
~~~~~~~~~~~~~~~~~~~~~~~~~~~~~

(transfer none with no expectation from the caller a floating ref should be returned)

In regards to vfuncs which imply returning an internal borrowed reference, we can detect potential
errors by again looking at the outgoing PyObject wrapper ref-count before returning the GObject.
Consider the following examples:

.. code-block:: python

    class SomethingAccessible(Atk.Object):
        def do_get_parent(self):
            return self.parent

.. code-block:: python

    class SomethingAccessible(Atk.Object):
        def do_get_parent(self):
            return ParentObject()

If the PyObject wrapper ref count is only one (latter case), it means Python would normally collect
this, so we need to increment the GObject ref count to keep Python from also destroying the GObject
and print a warning stating that this might be a leak. While this case may rarely happen in the
real world, it is a possibility bindings need to explicitly address in order to minimize future
support load.

Additional Considerations
-------------------------

Toggle References
~~~~~~~~~~~~~~~~~~

Beyond the situations given above, we also have the added complexity of toggle references in cases
where we need to keep the Python wrapper alive and carried along with the GObject through its
lifetime. This can occur when instance attributes are set on a PyObject wrapper:

.. code-block:: python

    btn = Gtk.Button()
    btn.some_custom_py_attr = 'foo'

In this case pygobject will detect the dictionary modification and add a toggle ref to keep the
wrapper alive along with the GObject.

Final Thoughts
--------------

Importantly, unittests for all the combinations of the above situations need to be written. There is
already some work being done in the following ticket to address this (687522_)

Analysis
--------

Some analysis of existing .gir files was accomplished by using the attached gir-query script which
you can give a directory of .gir files and query them using xpath.

VFuncs with Object Return
~~~~~~~~~~~~~~~~~~~~~~~~~

This shows a variety of the possibilities talked about above.

::

    $ gir-query --print-closure-arg-types "Object,GObject.Object,Widget,Gtk.Widget,Atk.Object" | grep "|| in ||"

==============================================================  ===============  ===========  ==============  ========
Closure Arg                                                     Closure Type     Direction    Arg Type        Transfer
==============================================================  ===============  ===========  ==============  ========
``Nautilus.LocationWidgetProvider.get_widget.window``           virtual-method   in           Gtk.Widget      none
``Nautilus.MenuProvider.get_background_items.window``           virtual-method   in           Gtk.Widget      none
``Nautilus.MenuProvider.get_file_items.window``                 virtual-method   in           Gtk.Widget      none
``GcrUi.ImportButton.imported.importer``                        signal           in           GObject.Object  none
``GcrUi.ImportButton.importing.importer``                       signal           in           GObject.Object  none
``Gtk.Buildable.add_child.child``                               virtual-method   in           GObject.Object  none
``Gtk.Buildable.custom_finished.child``                         virtual-method   in           GObject.Object  none
``Gtk.Buildable.custom_tag_end.child``                          virtual-method   in           GObject.Object  none
``Gtk.Buildable.custom_tag_start.child``                        virtual-method   in           GObject.Object  none
``Gtk.TextTag.event.event_object``                              virtual-method   in           GObject.Object  none
``Gtk.Action.connect_proxy.proxy``                              virtual-method   in           Widget          none
``Gtk.Action.disconnect_proxy.proxy``                           virtual-method   in           Widget          none
``Gtk.Assistant.prepare.page``                                  virtual-method   in           Widget          none
``Gtk.CellArea.activate.widget``                                virtual-method   in           Widget          none
``Gtk.CellArea.event.widget``                                   virtual-method   in           Widget          none
``Gtk.CellArea.foreach_alloc.widget``                           virtual-method   in           Widget          none
``Gtk.CellArea.get_preferred_height.widget``                    virtual-method   in           Widget          none
``Gtk.CellArea.get_preferred_height_for_width.widget``          virtual-method   in           Widget          none
``Gtk.CellArea.get_preferred_width.widget``                     virtual-method   in           Widget          none
``Gtk.CellArea.get_preferred_width_for_height.widget``          virtual-method   in           Widget          none
``Gtk.CellArea.render.widget``                                  virtual-method   in           Widget          none
``Gtk.CellRenderer.activate.widget``                            virtual-method   in           Widget          none
``Gtk.CellRenderer.get_aligned_area.widget``                    virtual-method   in           Widget          none
``Gtk.CellRenderer.get_preferred_height.widget``                virtual-method   in           Widget          none
``Gtk.CellRenderer.get_preferred_height_for_width.widget``      virtual-method   in           Widget          none
``Gtk.CellRenderer.get_preferred_width.widget``                 virtual-method   in           Widget          none
``Gtk.CellRenderer.get_preferred_width_for_height.widget``      virtual-method   in           Widget          none
``Gtk.CellRenderer.get_size.widget``                            virtual-method   in           Widget          none
``Gtk.CellRenderer.render.widget``                              virtual-method   in           Widget          none
``Gtk.CellRenderer.start_editing.widget``                       virtual-method   in           Widget          none
``Gtk.Container.add.widget``                                    virtual-method   in           Widget          none
``Gtk.Container.composite_name.child``                          virtual-method   in           Widget          none
``Gtk.Container.get_child_property.child``                      virtual-method   in           Widget          none
``Gtk.Container.get_path_for_child.child``                      virtual-method   in           Widget          none
``Gtk.Container.remove.widget``                                 virtual-method   in           Widget          none
``Gtk.Container.set_child_property.child``                      virtual-method   in           Widget          none
``Gtk.Container.set_focus_child.widget``                        virtual-method   in           Widget          none
``Gtk.HandleBox.child_attached.child``                          virtual-method   in           Widget          none
``Gtk.HandleBox.child_detached.child``                          virtual-method   in           Widget          none
``Gtk.List.select_child.child``                                 virtual-method   in           Widget          none
``Gtk.List.unselect_child.child``                               virtual-method   in           Widget          none
``Gtk.MenuShell.insert.child``                                  virtual-method   in           Widget          none
``Gtk.MenuShell.select_item.menu_item``                         virtual-method   in           Widget          none
``Gtk.Notebook.create_window.page``                             virtual-method   in           Widget          none
``Gtk.Notebook.insert_page.child``                              virtual-method   in           Widget          none
``Gtk.Notebook.insert_page.tab_label``                          virtual-method   in           Widget          none
``Gtk.Notebook.insert_page.menu_label``                         virtual-method   in           Widget          none
``Gtk.Notebook.page_added.child``                               virtual-method   in           Widget          none
``Gtk.Notebook.page_removed.child``                             virtual-method   in           Widget          none
``Gtk.Notebook.page_reordered.child``                           virtual-method   in           Widget          none
``Gtk.Notebook.switch_page.page``                               virtual-method   in           Widget          none
``Gtk.Overlay.get_child_position.widget``                       virtual-method   in           Widget          none
``Gtk.PrintOperation.custom_widget_apply.widget``               virtual-method   in           Widget          none
``Gtk.PrintOperation.update_custom_widget.widget``              virtual-method   in           Widget          none
``Gtk.Style.draw_arrow.widget``                                 virtual-method   in           Widget          none
``Gtk.Style.draw_box.widget``                                   virtual-method   in           Widget          none
``Gtk.Style.draw_box_gap.widget``                               virtual-method   in           Widget          none
``Gtk.Style.draw_check.widget``                                 virtual-method   in           Widget          none
``Gtk.Style.draw_diamond.widget``                               virtual-method   in           Widget          none
``Gtk.Style.draw_expander.widget``                              virtual-method   in           Widget          none
``Gtk.Style.draw_extension.widget``                             virtual-method   in           Widget          none
``Gtk.Style.draw_flat_box.widget``                              virtual-method   in           Widget          none
``Gtk.Style.draw_focus.widget``                                 virtual-method   in           Widget          none
``Gtk.Style.draw_handle.widget``                                virtual-method   in           Widget          none
``Gtk.Style.draw_hline.widget``                                 virtual-method   in           Widget          none
``Gtk.Style.draw_layout.widget``                                virtual-method   in           Widget          none
``Gtk.Style.draw_option.widget``                                virtual-method   in           Widget          none
``Gtk.Style.draw_polygon.widget``                               virtual-method   in           Widget          none
``Gtk.Style.draw_resize_grip.widget``                           virtual-method   in           Widget          none
``Gtk.Style.draw_shadow.widget``                                virtual-method   in           Widget          none
``Gtk.Style.draw_shadow_gap.widget``                            virtual-method   in           Widget          none
``Gtk.Style.draw_slider.widget``                                virtual-method   in           Widget          none
``Gtk.Style.draw_spinner.widget``                               virtual-method   in           Widget          none
``Gtk.Style.draw_string.widget``                                virtual-method   in           Widget          none
``Gtk.Style.draw_tab.widget``                                   virtual-method   in           Widget          none
``Gtk.Style.draw_vline.widget``                                 virtual-method   in           Widget          none
``Gtk.Style.render_icon.widget``                                virtual-method   in           Widget          none
``Gtk.TextLayout.allocate_child.child``                         virtual-method   in           Widget          none
``Gtk.TipsQuery.widget_entered.widget``                         virtual-method   in           Widget          none
``Gtk.TipsQuery.widget_selected.widget``                        virtual-method   in           Widget          none
``Gtk.UIManager.add_widget.widget``                             virtual-method   in           Widget          none
``Gtk.UIManager.connect_proxy.proxy``                           virtual-method   in           Widget          none
``Gtk.UIManager.disconnect_proxy.proxy``                        virtual-method   in           Widget          none
``Gtk.Widget.hierarchy_changed.previous_toplevel``              virtual-method   in           Widget          none
``Gtk.Widget.parent_set.previous_parent``                       virtual-method   in           Widget          none
``Gtk.Window.set_focus.focus``                                  virtual-method   in           Widget          none
``Gtk.TextLayout.allocate-child.object``                        signal           in           Object          none
``Gtk.AccelGroup.accel-activate.acceleratable``                 signal           in           GObject.Object  none
``Gtk.TextTag.event.object``                                    signal           in           GObject.Object  none
``Gtk.ActionGroup.connect-proxy.proxy``                         signal           in           Widget          none
``Gtk.ActionGroup.disconnect-proxy.proxy``                      signal           in           Widget          none
``Gtk.Assistant.prepare.page``                                  signal           in           Widget          none
``Gtk.Container.add.object``                                    signal           in           Widget          none
``Gtk.Container.remove.object``                                 signal           in           Widget          none
``Gtk.Container.set-focus-child.object``                        signal           in           Widget          none
``Gtk.HandleBox.child-attached.object``                         signal           in           Widget          none
``Gtk.HandleBox.child-detached.object``                         signal           in           Widget          none
``Gtk.List.select-child.object``                                signal           in           Widget          none
``Gtk.List.unselect-child.object``                              signal           in           Widget          none
``Gtk.Notebook.create-window.page``                             signal           in           Widget          none
``Gtk.Notebook.page-added.child``                               signal           in           Widget          none
``Gtk.Notebook.page-removed.child``                             signal           in           Widget          none
``Gtk.Notebook.page-reordered.child``                           signal           in           Widget          none
``Gtk.PrintOperation.custom-widget-apply.widget``               signal           in           Widget          none
``Gtk.PrintOperation.update-custom-widget.widget``              signal           in           Widget          none
``Gtk.TipsQuery.widget-entered.object``                         signal           in           Widget          none
``Gtk.TipsQuery.widget-selected.object``                        signal           in           Widget          none
``Gtk.UIManager.add-widget.widget``                             signal           in           Widget          none
``Gtk.UIManager.connect-proxy.proxy``                           signal           in           Widget          none
``Gtk.UIManager.disconnect-proxy.proxy``                        signal           in           Widget          none
``Gtk.Widget.hierarchy-changed.previous_toplevel``              signal           in           Widget          none
``Gtk.Widget.parent-set.old_parent``                            signal           in           Widget          none
``Gtk.Window.set-focus.object``                                 signal           in           Widget          none
``Gst.ControlBinding.sync_values.object``                       virtual-method   in           GObject.Object  none
``Gst.Object.deep_notify.orig``                                 virtual-method   in           GObject.Object  none
``Gst.ChildProxy.child_added.child``                            virtual-method   in           GObject.Object  none
``Gst.ChildProxy.child_removed.child``                          virtual-method   in           GObject.Object  none
``Gst.Object.deep-notify.prop_object``                          signal           in           GObject.Object  none
``Gst.ChildProxy.child-added.object``                           signal           in           GObject.Object  none
``Gst.ChildProxy.child-removed.object``                         signal           in           GObject.Object  none
``NMClient.DeviceWifi.access-point-added.ap``                   signal           in           GObject.Object  none
``NMClient.DeviceWifi.access-point-removed.ap``                 signal           in           GObject.Object  none
``NMClient.DeviceWimax.nsp-added.nsp``                          signal           in           GObject.Object  none
``NMClient.DeviceWimax.nsp-removed.nsp``                        signal           in           GObject.Object  none
``NMClient.RemoteSettings.new-connection.object``               signal           in           GObject.Object  none
``Gio.SocketService.incoming.source_object``                    virtual-method   in           GObject.Object  none
``Gio.ThreadedSocketService.run.source_object``                 virtual-method   in           GObject.Object  none
``Gio.SocketService.incoming.source_object``                    signal           in           GObject.Object  none
``Gio.ThreadedSocketService.run.source_object``                 signal           in           GObject.Object  none
``Json.Parser.object_end.object``                               virtual-method   in           Object          none
``Json.Parser.object_member.object``                            virtual-method   in           Object          none
``Json.Parser.object-end.object``                               signal           in           Object          none
``Json.Parser.object-member.object``                            signal           in           Object          none
``GnomeDesktop.BGCrossfade.finished.window``                    signal           in           GObject.Object  none
``Gladeui.Widget.replace_child``                                virtual-method   in           GObject.Object  none
``Gladeui.Widget.replace_child``                                virtual-method   in           GObject.Object  none
``Gladeui.BaseEditor.build_child``                              virtual-method   in           Widget          none
``Gladeui.BaseEditor.change_type``                              virtual-method   in           Widget          none
``Gladeui.BaseEditor.child_selected``                           virtual-method   in           Widget          none
``Gladeui.BaseEditor.delete_child``                             virtual-method   in           Widget          none
``Gladeui.BaseEditor.delete_child``                             virtual-method   in           Widget          none
``Gladeui.BaseEditor.get_display_name``                         virtual-method   in           Widget          none
``Gladeui.BaseEditor.move_child``                               virtual-method   in           Widget          none
``Gladeui.BaseEditor.move_child``                               virtual-method   in           Widget          none
``Gladeui.Editable.load.widget``                                virtual-method   in           Widget          none
``Gladeui.Project.add_object.widget``                           virtual-method   in           Widget          none
``Gladeui.Project.remove_object.widget``                        virtual-method   in           Widget          none
``Gladeui.Project.widget_name_changed.widget``                  virtual-method   in           Widget          none
``Gladeui.Widget.add_child``                                    virtual-method   in           Widget          none
``Gladeui.Widget.remove_child``                                 virtual-method   in           Widget          none
``Gladeui.App.signal-editor-created.signal_editor``             signal           in           GObject.Object  none
``Gladeui.App.widget-adaptor-registered.adaptor``               signal           in           GObject.Object  none
``Gladeui.BaseEditor.build-child.gparent``                      signal           in           GObject.Object  none
``Gladeui.BaseEditor.change-type.object``                       signal           in           GObject.Object  none
``Gladeui.BaseEditor.child-selected.gchild``                    signal           in           GObject.Object  none
``Gladeui.BaseEditor.delete-child.gparent``                     signal           in           GObject.Object  none
``Gladeui.BaseEditor.delete-child.gchild``                      signal           in           GObject.Object  none
``Gladeui.BaseEditor.get-display-name.gchild``                  signal           in           GObject.Object  none
``Gladeui.BaseEditor.move-child.gparent``                       signal           in           GObject.Object  none
``Gladeui.BaseEditor.move-child.gchild``                        signal           in           GObject.Object  none
``Gladeui.Project.add-widget.arg1``                             signal           in           Widget          none
``Gladeui.Project.remove-widget.arg1``                          signal           in           Widget          none
``Gladeui.Project.widget-name-changed.arg1``                    signal           in           Widget          none
``Gladeui.Project.widget-visibility-changed.widget``            signal           in           Widget          none
``Gck.Module.authenticate_object.object``                       virtual-method   in           Object          none
``Gck.Module.authenticate-object.object``                       signal           in           Object          none
``Soup.Session.connection-created.connection``                  signal           in           GObject.Object  none
``Soup.Session.tunneling.connection``                           signal           in           GObject.Object  none
``Peas.ExtensionSet.extension-added.exten``                     signal           in           GObject.Object  none
``Peas.ExtensionSet.extension-removed.exten``                   signal           in           GObject.Object  none
``Gcr.Collection.added.object``                                 virtual-method   in           GObject.Object  none
``Gcr.Collection.contains.object``                              virtual-method   in           GObject.Object  none
``Gcr.Collection.removed.object``                               virtual-method   in           GObject.Object  none
``Atk.Object.set_parent.parent``                                virtual-method   in           Object          none
``Atk.Table.set_caption.caption``                               virtual-method   in           Object          none
``Atk.Table.set_column_header.header``                          virtual-method   in           Object          none
``Atk.Table.set_row_header.header``                             virtual-method   in           Object          none
``Atk.Table.set_summary.accessible``                            virtual-method   in           Object          none
==============================================================  ===============  ===========  ==============  ========

::

    $ gir-query --print-closure-arg-types "Object,GObject.Object,Widget,Gtk.Widget,Atk.Object" | grep "|| out ||"

==============================================================  ===============  ===========  ==============  ========
Closure Arg                                                     Closure Type     Direction    Arg Type        Transfer
==============================================================  ===============  ===========  ==============  ========
``GtkSource.CompletionProvider.get_info_widget``                virtual-method   out          Gtk.Widget      none
``PeasGtk.Configurable.create_configure_widget``                virtual-method   out          Gtk.Widget      full
``Nautilus.LocationWidgetProvider.get_widget``                  virtual-method   out          Gtk.Widget      none
``Gtk.Buildable.construct_child``                               virtual-method   out          GObject.Object  full
``Gtk.Buildable.get_internal_child``                            virtual-method   out          GObject.Object  none
``Gtk.Action.create_menu``                                      virtual-method   out          Widget          none
``Gtk.Action.create_menu_item``                                 virtual-method   out          Widget          none
``Gtk.Action.create_tool_item``                                 virtual-method   out          Widget          none
``Gtk.PrintOperation.create_custom_widget``                     virtual-method   out          Widget          None
``Gtk.UIManager.get_widget``                                    virtual-method   out          Widget          none
``Gtk.Widget.get_accessible``                                   virtual-method   out          Atk.Object      none
``Gtk.PrintOperation.create-custom-widget``                     signal           out          GObject.Object  none
``Gst.ChildProxy.get_child_by_index``                           virtual-method   out          GObject.Object  full
``Gst.ChildProxy.get_child_by_name``                            virtual-method   out          GObject.Object  full
``Gio.AsyncResult.get_source_object``                           virtual-method   out          GObject.Object  full
``Gladeui.BaseEditor.build_child``                              virtual-method   out          Widget          None
``Gladeui.EditorProperty.create_input``                         virtual-method   out          Gtk.Widget      None
``Gladeui.BaseEditor.build-child``                              signal           out          GObject.Object  None
``Atk.Component.ref_accessible_at_point``                       virtual-method   out          Object          full
``Atk.Hyperlink.get_object``                                    virtual-method   out          Object          none
``Atk.Object.get_parent``                                       virtual-method   out          Object          none
``Atk.Object.ref_child``                                        virtual-method   out          Object          None
``Atk.Selection.ref_selection``                                 virtual-method   out          Object          full
``Atk.Table.get_caption``                                       virtual-method   out          Object          none
``Atk.Table.get_column_header``                                 virtual-method   out          Object          none
``Atk.Table.get_row_header``                                    virtual-method   out          Object          none
``Atk.Table.get_summary``                                       virtual-method   out          Object          full
``Atk.Table.ref_at``                                            virtual-method   out          Object          full
==============================================================  ===============  ===========  ==============  ========
