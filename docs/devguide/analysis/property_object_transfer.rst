Property Object Transfer Analysis
=================================

**GIR**                                                     **Code**

==========================================================  ===============  ===========  =========================  ========
**Property**                                                **Type**         Transfer     **get**                    **set**
==========================================================  ===============  ===========  =========================  ========
``GObject.Binding.source``                                  Object           none         adds ref                   steals ref
``GObject.Binding.target``                                  Object           none         adds ref                   steals ref
``Nautilus.PropertyPage.label``                             Gtk.Widget       none         adds ref                   adds ref
``Nautilus.PropertyPage.page``                              Gtk.Widget       none         adds ref                   adds ref
``Gtk.AccelLabel.accel-widget``                             Widget           full         adds ref                   adds weak-ref
``Gtk.Accessible.widget``                                   Widget           full         adds ref                   depends on implementation of widget_set
``Gtk.Button.image``                                        Widget           none         adds ref                   steals ref
``Gtk.ColorSelectionDialog.cancel-button``                  Widget           full         adds ref                   N/A
``Gtk.ColorSelectionDialog.color-selection``                Widget           full         adds ref                   N/A
``Gtk.ColorSelectionDialog.help-button``                    Widget           full         adds ref                   N/A
``Gtk.ColorSelectionDialog.ok-button``                      Widget           full         adds ref                   N/A
``Gtk.Container.child``                                     Widget           full         N/A                        depends on "add" closure implementation
``Gtk.Expander.label-widget``                               Widget           full         adds ref                   sinks/adds ref
``Gtk.FileChooser.extra-widget``                            Widget           full         depends on implementation  depends on implementation
``Gtk.FileChooser.preview-widget``                          Widget           full         depends on implementation  depends on implementation
``Gtk.Frame.label-widget``                                  Widget           full         adds ref                   sinks/adds ref
``Gtk.ImageMenuItem.image``                                 Widget           full         adds ref                   sinks/adds ref
``Gtk.Label.mnemonic-widget``                               Widget           full         adds ref                   adds weak-ref
``Gtk.Menu.attach-widget``                                  Widget           none         adds ref                   nothing (sinks parent menu)
``Gtk.MessageDialog.image``                                 Widget           none         adds ref                   depends on "add" closure implementation
``Gtk.MessageDialog.message-area``                          Widget           none         adds ref                   N/A
``Gtk.ToolButton.icon-widget``                              Widget           full         adds ref                   sinks/adds ref
``Gtk.ToolButton.label-widget``                             Widget           full         adds ref                   sinks/adds ref
``Gtk.ToolItemGroup.label-widget``                          Widget           full
``Gtk.TreeViewColumn.widget``                               Widget           full         adds ref                   sinks/adds ref
``Gtk.Window.attached-to``                                  Widget           none         adds ref                   adds ref
``Gst.ControlBinding.object``                               Object           none         adds ref                   adds weak-ref
``Gst.Object.parent``                                       Object           none         adds ref                   adds/sinks ref
==========================================================  ===============  ===========  =========================  ========

==========================================================  =========================  ===========
**Property**                                                **Type**                   Transfer
==========================================================  =========================  ===========
``GObject.Binding.source``                                  Object                     none
``GObject.Binding.target``                                  Object                     none
``GtkSource.Completion.view``                               View                       none
``GtkSource.Gutter.view``                                   View                       none
``GtkSource.GutterRenderer.view``                           Gtk.TextView               none
``GtkSource.CompletionContext.completion``                  Completion                 none
``GtkSource.View.completion``                               Completion                 none
``GtkSource.PrintCompositor.buffer``                        Buffer                     none
``GtkSource.Buffer.style-scheme``                           StyleScheme                none
``GtkSource.CompletionItem.icon``                           GdkPixbuf.Pixbuf           none
``GtkSource.CompletionWords.icon``                          GdkPixbuf.Pixbuf           none
``GtkSource.GutterRendererPixbuf.pixbuf``                   GdkPixbuf.Pixbuf           none
``GtkSource.MarkAttributes.pixbuf``                         GdkPixbuf.Pixbuf           none
``GtkSource.Buffer.language``                               Language                   none
``PeasGtk.PluginManager.view``                              PluginManagerView          none
``PeasGtk.PluginManager.engine``                            Peas.Engine                none
``PeasGtk.PluginManagerView.engine``                        Peas.Engine                none
``Nautilus.MenuItem.menu``                                  Menu                       none
``Nautilus.PropertyPage.label``                             Gtk.Widget                 none
``Nautilus.PropertyPage.page``                              Gtk.Widget                 none
``GcrUi.ViewerWidget.parser``                               Gcr.Parser                 none
``GcrUi.KeyRenderer.object``                                Gck.Object                 none
``Tracker.SparqlCursor.connection``                         Tracker.SparqlConnection   None
``Rest.ProxyCall.proxy``                                    Proxy                      none
``Gtk.Entry.buffer``                                        EntryBuffer                none
``Gtk.RadioMenuItem.group``                                 RadioMenuItem              none
``Gtk.Application.active-window``                           Window                     none
``Gtk.MountOperation.parent``                               Window                     none
``Gtk.Window.transient-for``                                Window                     none
``Gtk.MenuButton.popup``                                    Menu                       none
``Gtk.MenuItem.submenu``                                    Menu                       none
``Gtk.MenuToolButton.menu``                                 Menu                       none
``Gtk.Window.application``                                  Application                none
``Gtk.StyleContext.paint-clock``                            Gdk.FrameClock             none
``Gtk.MenuButton.align-widget``                             Container                  none
``Gtk.Widget.parent``                                       Container                  none
``Gtk.CellAreaContext.area``                                CellArea                   none
``Gtk.CellView.cell-area``                                  CellArea                   none
``Gtk.ComboBox.cell-area``                                  CellArea                   none
``Gtk.EntryCompletion.cell-area``                           CellArea                   none
``Gtk.IconView.cell-area``                                  CellArea                   none
``Gtk.TreeViewColumn.cell-area``                            CellArea                   none
``Gtk.Entry.completion``                                    EntryCompletion            none
``Gtk.AccelLabel.accel-widget``                             Widget                     none
``Gtk.Accessible.widget``                                   Widget                     none
``Gtk.Button.image``                                        Widget                     none
``Gtk.ColorSelectionDialog.cancel-button``                  Widget                     none
``Gtk.ColorSelectionDialog.color-selection``                Widget                     none
``Gtk.ColorSelectionDialog.help-button``                    Widget                     none
``Gtk.ColorSelectionDialog.ok-button``                      Widget                     none
``Gtk.Container.child``                                     Widget                     none
``Gtk.Expander.label-widget``                               Widget                     none
``Gtk.FileChooser.extra-widget``                            Widget                     none
``Gtk.FileChooser.preview-widget``                          Widget                     none
``Gtk.Frame.label-widget``                                  Widget                     none
``Gtk.ImageMenuItem.image``                                 Widget                     none
``Gtk.Label.mnemonic-widget``                               Widget                     none
``Gtk.Menu.attach-widget``                                  Widget                     none
``Gtk.MessageDialog.image``                                 Widget                     none
``Gtk.MessageDialog.message-area``                          Widget                     none
``Gtk.ToolButton.icon-widget``                              Widget                     none
``Gtk.ToolButton.label-widget``                             Widget                     none
``Gtk.ToolItemGroup.label-widget``                          Widget                     none
``Gtk.TreeViewColumn.widget``                               Widget                     none
``Gtk.Window.attached-to``                                  Widget                     none
``Gtk.RecentChooser.recent-manager``                        RecentManager              none
``Gtk.Application.app-menu``                                Gio.MenuModel              none
``Gtk.Application.menubar``                                 Gio.MenuModel              none
``Gtk.MenuButton.menu-model``                               Gio.MenuModel              none
``Gtk.TextBuffer.tag-table``                                TextTagTable               none
``Gtk.RadioButton.group``                                   RadioButton                none
``Gtk.CellArea.edited-cell``                                CellRenderer               none
``Gtk.CellArea.focus-cell``                                 CellRenderer               none
``Gtk.NumerableIcon.style-context``                         StyleContext               none
``Gtk.Style.context``                                       StyleContext               none
``Gtk.StyleContext.parent``                                 StyleContext               none
``Gtk.Image.pixbuf-animation``                              GdkPixbuf.PixbufAnimation  none
``Gtk.Widget.style``                                        Style                      none
``Gtk.RadioAction.group``                                   RadioAction                none
``Gtk.PrintOperation.default-page-setup``                   PageSetup                  none
``Gtk.Invisible.screen``                                    Gdk.Screen                 none
``Gtk.MountOperation.screen``                               Gdk.Screen                 none
``Gtk.StatusIcon.screen``                                   Gdk.Screen                 none
``Gtk.StyleContext.screen``                                 Gdk.Screen                 none
``Gtk.Window.screen``                                       Gdk.Screen                 none
``Gtk.TreeView.expander-column``                            TreeViewColumn             none
``Gtk.ActionGroup.accel-group``                             AccelGroup                 none
``Gtk.ImageMenuItem.accel-group``                           AccelGroup                 none
``Gtk.Menu.accel-group``                                    AccelGroup                 none
``Gtk.CellView.cell-area-context``                          CellAreaContext            none
``Gtk.TextView.buffer``                                     TextBuffer                 none
``Gtk.CellRendererSpin.adjustment``                         Adjustment                 none
``Gtk.Range.adjustment``                                    Adjustment                 none
``Gtk.ScaleButton.adjustment``                              Adjustment                 none
``Gtk.Scrollable.hadjustment``                              Adjustment                 none
``Gtk.Scrollable.vadjustment``                              Adjustment                 none
``Gtk.ScrolledWindow.hadjustment``                          Adjustment                 none
``Gtk.ScrolledWindow.vadjustment``                          Adjustment                 none
``Gtk.SpinButton.adjustment``                               Adjustment                 none
``Gtk.Activatable.related-action``                          Action                     none
``Gtk.RadioToolButton.group``                               RadioToolButton            none
``Gtk.PrintOperation.print-settings``                       PrintSettings              none
``Gtk.Action.action-group``                                 ActionGroup                none
``Gtk.AboutDialog.logo``                                    GdkPixbuf.Pixbuf           none
``Gtk.CellRendererPixbuf.pixbuf``                           GdkPixbuf.Pixbuf           none
``Gtk.CellRendererPixbuf.pixbuf-expander-closed``           GdkPixbuf.Pixbuf           none
``Gtk.CellRendererPixbuf.pixbuf-expander-open``             GdkPixbuf.Pixbuf           none
``Gtk.Entry.primary-icon-pixbuf``                           GdkPixbuf.Pixbuf           none
``Gtk.Entry.secondary-icon-pixbuf``                         GdkPixbuf.Pixbuf           none
``Gtk.Image.pixbuf``                                        GdkPixbuf.Pixbuf           none
``Gtk.StatusIcon.pixbuf``                                   GdkPixbuf.Pixbuf           none
``Gtk.Window.icon``                                         GdkPixbuf.Pixbuf           none
``Gtk.Plug.socket-window``                                  Gdk.Window                 none
``Gtk.Widget.window``                                       Gdk.Window                 none
``Gtk.FileChooser.filter``                                  FileFilter                 none
``Gtk.LockButton.permission``                               Gio.Permission             none
``Gtk.RecentChooser.filter``                                RecentFilter               none
``Gst.Pad.template``                                        PadTemplate                none
``Gst.ControlBinding.object``                               Object                     none
``Gst.Object.parent``                                       Object                     none
``GtkClutter.Actor.contents``                               Gtk.Widget                 none
``GstController.ARGBControlBinding.control-source-a``       Gst.ControlSource          none
``GstController.ARGBControlBinding.control-source-b``       Gst.ControlSource          none
``GstController.ARGBControlBinding.control-source-g``       Gst.ControlSource          none
``GstController.ARGBControlBinding.control-source-r``       Gst.ControlSource          none
``GstController.DirectControlBinding.control-source``       Gst.ControlSource          none
``Gtk.OptionMenu.menu``                                     Menu                       none
``Gtk.TipsQuery.caller``                                    Widget                     none
``Gtk.Layout.hadjustment``                                  Adjustment                 none
``Gtk.Layout.vadjustment``                                  Adjustment                 none
``Gtk.ProgressBar.adjustment``                              Adjustment                 none
``Gtk.TreeView.hadjustment``                                Adjustment                 none
``Gtk.TreeView.vadjustment``                                Adjustment                 none
``Gtk.Viewport.hadjustment``                                Adjustment                 none
``Gtk.Viewport.vadjustment``                                Adjustment                 none
``Vte.Terminal.pty-object``                                 Pty                        none
``Vte.Terminal.background-image-pixbuf``                    GdkPixbuf.Pixbuf           none
``Gdk.Window.cursor``                                       Cursor                     none
``Gdk.PangoRenderer.screen``                                Screen                     none
``Gdk.DisplayManager.default-display``                      Display                    none
``Clutter.Actor.effect``                                    Effect                     none
``Clutter.StageManager.default-stage``                      Stage                      none
``Clutter.InputDevice.device-manager``                      DeviceManager              none
``Clutter.BehaviourPath.path``                              Path                       none
``Clutter.PathConstraint.path``                             Path                       none
``Clutter.Transition.interval``                             Interval                   none
``Clutter.ChildMeta.container``                             Container                  none
``Clutter.Text.buffer``                                     TextBuffer                 none
``Clutter.Animation.object``                                GObject.Object             none
``Clutter.Actor.actions``                                   Action                     none
``Clutter.Actor.first-child``                               Actor                      none
``Clutter.Actor.last-child``                                Actor                      none
``Clutter.ActorMeta.actor``                                 Actor                      none
``Clutter.AlignConstraint.source``                          Actor                      none
``Clutter.BindConstraint.source``                           Actor                      none
``Clutter.ChildMeta.actor``                                 Actor                      none
``Clutter.Clone.source``                                    Actor                      none
``Clutter.DragAction.drag-handle``                          Actor                      none
``Clutter.SnapConstraint.source``                           Actor                      none
``Clutter.Stage.key-focus``                                 Actor                      none
``Clutter.Actor.constraints``                               Constraint                 none
``Clutter.Animation.alpha``                                 Alpha                      none
``Clutter.Behaviour.alpha``                                 Alpha                      none
``Clutter.Alpha.timeline``                                  Timeline                   none
``Clutter.Animation.timeline``                              Timeline                   none
``Clutter.Animator.timeline``                               Timeline                   none
``Clutter.ModelIter.model``                                 Model                      none
``Clutter.DeviceManager.backend``                           Backend                    none
``Clutter.InputDevice.backend``                             Backend                    none
``Clutter.Settings.backend``                                Backend                    none
``Clutter.Actor.layout-manager``                            LayoutManager              none
``Clutter.LayoutMeta.manager``                              LayoutManager              none
``Gdk.Device.associated-device``                            Device                     none
``Gdk.Device.device-manager``                               DeviceManager              none
``Gdk.AppLaunchContext.display``                            Display                    none
``Gdk.Cursor.display``                                      Display                    none
``Gdk.Device.display``                                      Display                    none
``Gdk.DeviceManager.display``                               Display                    none
``SecretUnstable.Collection.service``                       Service                    none
``SecretUnstable.Item.service``                             Service                    none
``GUsb.Device.context``                                     Context                    none
``GUsb.DeviceList.context``                                 Context                    none
``NMClient.Device.dhcp4-config``                            DHCP4Config                none
``NMClient.DeviceWimax.active-nsp``                         WimaxNsp                   none
``NMClient.Device.ip4-config``                              IP4Config                  none
``NMClient.DeviceWifi.active-access-point``                 AccessPoint                none
``NMClient.Device.active-connection``                       ActiveConnection           none
``NMClient.Device.ip6-config``                              IP6Config                  none
``NMClient.DeviceOlpcMesh.companion``                       DeviceWifi                 none
``NMClient.Device.dhcp6-config``                            DHCP6Config                none
``GstNet.NetTimeProvider.clock``                            Gst.Clock                  none
``Gio.SocketConnection.socket``                             Socket                     none
``Gio.UnixCredentialsMessage.credentials``                  Credentials                none
``Gio.TlsConnection.database``                              TlsDatabase                none
``Gio.FilterOutputStream.base-stream``                      OutputStream               none
``Gio.IOStream.output-stream``                              OutputStream               none
``Gio.DBusConnection.authentication-observer``              DBusAuthObserver           none
``Gio.DBusServer.authentication-observer``                  DBusAuthObserver           none
``Gio.DBusConnection.stream``                               IOStream                   none
``Gio.TcpWrapperConnection.base-io-stream``                 IOStream                   none
``Gio.TlsConnection.base-io-stream``                        IOStream                   none
``Gio.ZlibCompressor.file-info``                            FileInfo                   none
``Gio.ZlibDecompressor.file-info``                          FileInfo                   none
``Gio.Emblem.icon``                                         GObject.Object             none
``Gio.TlsCertificate.issuer``                               TlsCertificate             none
``Gio.TlsConnection.certificate``                           TlsCertificate             none
``Gio.TlsConnection.peer-certificate``                      TlsCertificate             none
``Gio.DBusObjectManagerClient.connection``                  DBusConnection             none
``Gio.DBusObjectManagerServer.connection``                  DBusConnection             none
``Gio.DBusObjectProxy.g-connection``                        DBusConnection             none
``Gio.DBusProxy.g-connection``                              DBusConnection             none
``Gio.InetAddressMask.address``                             InetAddress                none
``Gio.InetSocketAddress.address``                           InetAddress                none
``Gio.Application.action-group``                            ActionGroup                none
``Gio.UnixFDMessage.fd-list``                               UnixFDList                 none
``Gio.FilterInputStream.base-stream``                       InputStream                none
``Gio.IOStream.input-stream``                               InputStream                none
``Gio.Socket.local-address``                                SocketAddress              none
``Gio.Socket.remote-address``                               SocketAddress              none
``Gio.SocketClient.local-address``                          SocketAddress              none
``Gio.TlsConnection.interaction``                           TlsInteraction             none
``GnomeDesktop.IdleMonitor.device``                         Gdk.Device                 none
``GnomeDesktop.RRScreen.gdk-screen``                        Gdk.Screen                 none
``GnomeDesktop.RRConfig.screen``                            RRScreen                   none
``Gladeui.DesignView.project``                              Project                    none
``Gladeui.Inspector.project``                               Project                    none
``Gladeui.Palette.project``                                 Project                    none
``Gladeui.Widget.project``                                  Project                    none
``Gladeui.Editor.widget``                                   Widget                     none
``Gladeui.SignalModel.widget``                              Widget                     none
``Gladeui.Widget.parent``                                   Widget                     none
``Gladeui.Widget.template``                                 Widget                     none
``Gladeui.BaseEditor.container``                            GObject.Object             none
``Gladeui.Widget.object``                                   GObject.Object             none
``Gladeui.Project.add-item``                                WidgetAdaptor              none
``Gladeui.Widget.adaptor``                                  WidgetAdaptor              none
``Gck.Object.module``                                       Module                     none
``Gck.Password.module``                                     Module                     none
``Gck.Session.module``                                      Module                     none
``Gck.Slot.module``                                         Module                     none
``Gck.Object.session``                                      Session                    none
``Gck.Password.key``                                        Object                     none
``Gck.Password.token``                                      Slot                       none
``Gck.Session.slot``                                        Slot                       none
``Gck.Enumerator.interaction``                              Gio.TlsInteraction         none
``Gck.Session.interaction``                                 Gio.TlsInteraction         none
``Gck.Enumerator.chained``                                  Enumerator                 none
``Soup.MultipartInputStream.message``                       Message                    none
``Soup.Request.session``                                    Session                    none
``Soup.Session.tls-database``                               Gio.TlsDatabase            none
``Soup.Message.tls-certificate``                            Gio.TlsCertificate         none
``Soup.Server.tls-certificate``                             Gio.TlsCertificate         none
``Soup.Socket.tls-certificate``                             Gio.TlsCertificate         none
``Soup.Server.interface``                                   Address                    none
``Soup.Session.local-address``                              Address                    none
``Soup.Socket.local-address``                               Address                    none
``Soup.Socket.remote-address``                              Address                    none
``Peas.ExtensionSet.engine``                                Engine                     none
``Peas.Activatable.object``                                 GObject.Object             none
``Gcr.SystemPrompt.secret-exchange``                        SecretExchange             none
``Gcr.CertificateRequest.private-key``                      Gck.Object                 none
``Gcr.FilterCollection.underlying``                         Collection                 none
``Gcr.Importer.interaction``                                Gio.TlsInteraction         none
``Atk.Object.accessible-parent``                            Object                     none
``Atk.Object.accessible-table-caption-object``              Object                     none
``Atk.Object.accessible-table-column-header``               Object                     none
``Atk.Object.accessible-table-row-header``                  Object                     none
``Atk.Object.accessible-table-summary``                     Object                     none
==========================================================  =========================  ===========
