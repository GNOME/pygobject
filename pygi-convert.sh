#!/bin/sh

for f in $@; do
    sed -i "
    s/import gconf/import GConf/g
    s/gconf\./GConf\./g
    s/GConf\.client_get_default/GConf.Client.get_default/g

    s/import gtk/import Gtk/g
    s/gtk\./Gtk\./g
    s/Gtk.SIZE_GROUP_/Gtk.SizeGroupMode./g
    s/Gtk.POLICY_/Gtk.PolicyType./g
    s/Gtk.STATE_/Gtk.StateType./g
    s/Gtk.TARGET_/Gtk.TargetFlags./g
    s/Gtk.SHADOW_NONE/Gtk.ShadowType.NONE/g
    s/Gtk.ICON_SIZE_/Gtk.IconSize./g
    s/Gtk.IMAGE_/Gtk.ImageType./g

    s/Gtk.settings_get_default/Gtk.Settings.get_default/g
    s/Gtk.icon_theme_get_default/Gtk.IconTheme.get_default/g
    s/.window.set_type_hint/.set_type_hint/g
    s/self.drag_source_unset()/Gtk.drag_source_unset(self)/g
    s/self.drag_dest_unset()/Gtk.drag_dest_unset(self)/g
    s/Gtk.ScrolledWindow()/Gtk.ScrolledWindow(None, None)/g
    s/\.child/.get_child()/g

    s/pack_start(\([^,]*\))/pack_start(\1, expand=True, fill=True, padding=0)/g
    s/pack_start(\([^,]*\), fill=\([^,]*\))/pack_start(\1, expand=True, fill=\2, padding=0)/g
    s/pack_start(\([^,]*\), expand=\([^,]*\))/pack_start(\1, expand=\2, fill=True, padding=0)/g
    s/Gtk.HBox()/Gtk.HBox(homogeneous=False, spacing=0)/g
    s/Gtk.VBox()/Gtk.VBox(homogeneous=False, spacing=0)/g
    s/Gtk.Label()/Gtk.Label('')/g

    s/Gtk\..*\.__init__/gobject.GObject.__init__/g

    s/Gtk.gdk\./Gdk\./g
    s/Gdk.screen_width/Gdk.Screen.width/g
    s/Gdk.screen_height/Gdk.Screen.height/g
    s/Gdk.screen_get_default/Gdk.Screen.get_default/g
    s/Gdk.WINDOW_TYPE_HINT_/Gdk.WindowTypeHint./g
    s/Gdk\.Rectangle/Gdk.rectangle_new/g
    s/Gdk.BUTTON_PRESS_MASK/Gdk.EventMask.BUTTON_PRESS_MASK/g
    s/Gdk.POINTER_MOTION_HINT_MASK/Gdk.EventMask.POINTER_MOTION_HINT_MASK/g

    s/import pango/import Pango/g
    s/pango\./Pango\./g
    s/Pango\.FontDescription/Pango\.Font\.description_from_string/g
    s/Pango.ELLIPSIZE_/Pango.EllipsizeMode./g

    s/import hippo/import Hippo/g
    s/hippo\./Hippo\./g
    s/Hippo\..*\.__init__/gobject.GObject.__init__/g

    s/self._box.append(\([^,]*\))/self._box.append(\1, 0)/g
    s/self._box.sort(\([^,]*\))/self._box.sort(\1, None)/g

    s/import gio/import Gio/g
    s/gio\./Gio\./g

    s/import wnck/import Wnck/g
    s/wnck\./Wnck\./g
    s/Wnck.screen_get_default/Wnck.Screen.get_default/g
    " $f
done

echo 'Add "import Gdk" to'
rgrep -l Gdk\. $(find . -iname \*.py) | xargs grep -nL import\ Gdk

echo 'Add "import gobject" to'
rgrep -l gobject\. $(find . -iname \*.py) | xargs grep -nL import\ gobject

