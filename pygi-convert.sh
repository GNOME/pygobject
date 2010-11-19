#!/bin/sh

FILES_TO_CONVERT="$(find sugar-pygi -iname \*.py) $(find sugar-toolkit-pygi -iname \*.py) sugar-pygi/bin/sugar-session"

for f in $FILES_TO_CONVERT; do
    perl -i -0 \
    -pe "s/import gconf\n/from gi.repository import GConf\n/g;" \
    -pe "s/gconf\./GConf\./g;" \
    -pe "s/GConf\.client_get_default/GConf.Client.get_default/g;" \
    -pe "s/GConf\.CLIENT_/GConf.ClientPreloadType./g;" \
    -pe "s/GConf\.VALUE_/GConf.ValueType./g;" \
    -pe "s/gconf_client.notify_add\('\/desktop\/sugar\/collaboration\/publish_gadget',/return;gconf_client.notify_add\('\/desktop\/sugar\/collaboration\/publish_gadget',/g;" \
\
    -pe "s/import gtk\n/from gi.repository import Gtk\n/g;" \
    -pe "s/gtk\./Gtk\./g;" \
    -pe "s/Gtk.SIZE_GROUP_/Gtk.SizeGroupMode./g;" \
    -pe "s/Gtk.POLICY_/Gtk.PolicyType./g;" \
    -pe "s/Gtk.STATE_/Gtk.StateType./g;" \
    -pe "s/Gtk.TARGET_/Gtk.TargetFlags./g;" \
    -pe "s/Gtk.SHADOW_NONE/Gtk.ShadowType.NONE/g;" \
    -pe "s/Gtk.ICON_SIZE_/Gtk.IconSize./g;" \
    -pe "s/Gtk.IMAGE_/Gtk.ImageType./g;" \
    -pe "s/Gtk.SELECTION_/Gtk.SelectionMode./g;" \
    -pe "s/Gtk.CELL_RENDERER_MODE_/Gtk.CellRendererMode./g;" \
    -pe "s/Gtk.TREE_VIEW_COLUMN_/Gtk.TreeViewColumnSizing./g;" \
    -pe "s/Gtk.TEXT_DIR_/Gtk.TextDirection./g;" \
    -pe "s/Gtk.POS_/Gtk.PositionType./g;" \
    -pe "s/Gtk.SHADOW_/Gtk.ShadowType./g;" \
    -pe "s/Gtk.BUTTONBOX_/Gtk.ButtonBoxStyle./g;" \
    -pe "s/Gtk.SHRINK/Gtk.AttachOptions.SHRINK/g;" \
    -pe "s/Gtk.FILL/Gtk.AttachOptions.FILL/g;" \
    -pe "s/Gtk.JUSTIFY_/Gtk.Justification./g;" \
    -pe "s/Gtk.RESPONSE_/Gtk.ResponseType./g;" \
    -pe "s/Gtk.CORNER_/Gtk.CornerType./g;" \
    -pe "s/Gtk.ENTRY_ICON_/Gtk.EntryIconPosition./g;" \
    -pe "s/Gtk.MESSAGE_/Gtk.MessageType./g;" \
    -pe "s/Gtk.BUTTONS_/Gtk.ButtonsType./g;" \
    -pe "s/Gtk.settings_get_default/Gtk.Settings.get_default/g;" \
    -pe "s/Gtk.icon_theme_get_default/Gtk.IconTheme.get_default/g;" \
    -pe "s/.window.set_type_hint/.set_type_hint/g;" \
    -pe "s/self.drag_source_unset\(\)/Gtk.drag_source_unset\(self\)/g;" \
    -pe "s/self.drag_dest_unset\(\)/Gtk.drag_dest_unset\(self\)/g;" \
    -pe "s/Gtk.ListStore\(([^\)]*)\)/Gtk.ListStore.newv\(\[\1\]\)/g;" \
    -pe "s/Gtk.Alignment\(/Gtk.Alignment.new\(/g;" \
    -pe "s/self._model.filter_new\(\)/Gtk.TreeModelFilter.new\(self._model, None\)/g;" \
    -pe "#s/Gtk.ScrolledWindow\(\)/Gtk.ScrolledWindow\(None, None\)/g;" \
    -pe "#s/Gtk.Window.__init__\(self\)/Gtk.Window.__init__\(Gtk.WindowType.TOPLEVEL\)/g;" \
    -pe "s/\.child([^_a-z])/.get_child\(\)\1/g;" \
\
    -pe "s/column.pack_start\(([^,\)]*)\)/column.pack_start\(\1, True\)/g;" \
    -pe "s/pack_start\(([^,\)]*)\)/pack_start\(\1, True, True, 0\)/g;" \
    -pe "s/pack_start\(([^,]*), fill=([^,\)]*)\)/pack_start\(\1, True, \2, 0\)/g;" \
    -pe "s/pack_start\(([^,]*), expand=([^,\)]*)\)/pack_start\(\1, \2, True, 0\)/g;" \
    -pe "s/pack_start\(([^,]*),(\s*)padding=([A-Za-z0-9._]*)\)/pack_start\(\1,\2True, True,\2\3\)/g;" \
    -pe "#s/Gtk.HBox\(\)/Gtk.HBox\(False, 0\)/g;" \
    -pe "#s/Gtk.VBox\(\)/Gtk.VBox\(False, 0\)/g;" \
    -pe "s/Gtk.Label\(([^,\)]+)\)/Gtk.Label\(label=\1\)/g;" \
    -pe "s/Gtk.AccelLabel\(([^,\)]+)\)/Gtk.AccelLabel\(label=\1\)/g;" \
    -pe "s/len\(self._content.get_children\(\)\) > 0/self._content.get_children\(\)/g;" \
    -pe "s/len\(self.menu.get_children\(\)\) > 0/self.menu.get_children\(\)/g;" \
    -pe "s/([^\.^ ]*)\.drag_dest_set\(/Gtk.drag_dest_set\(\1, /g;" \
    -pe "s/Gtk\..*\.__init__/gobject.GObject.__init__/g;" \
\
    -pe "s/Gtk.gdk.x11_/GdkX11\./g;" \
    -pe "s/Gtk.gdk\./Gdk\./g;" \
    -pe "s/Gdk.screen_width/Gdk.Screen.width/g;" \
    -pe "s/Gdk.screen_height/Gdk.Screen.height/g;" \
    -pe "s/Gdk.screen_get_default/Gdk.Screen.get_default/g;" \
    -pe "s/Gdk.display_get_default/Gdk.Display.get_default/g;" \
    -pe "s/screen_, x_, y_, modmask = display.get_pointer\(\)/x_, y_, modmask = display.get_pointer\(None\)/g;" \
    -pe "s/Gdk.WINDOW_TYPE_HINT_/Gdk.WindowTypeHint./g;" \
    -pe "s/Gdk.MOD1_MASK/Gdk.ModifierType.MOD1_MASK/g;" \
    -pe "s/Gdk.([A-Z_0-9]*)_MASK/Gdk.EventMask.\1_MASK/g;" \
    -pe "s/Gdk.VISIBILITY_FULLY_OBSCURED/Gdk.VisibilityState.FULLY_OBSCURED/g;" \
    -pe "s/Gdk.BUTTON_PRESS/Gdk.EventType.BUTTON_PRESS/g;" \
    -pe "s/#Gdk.Rectangle\(([^,\)]*), ([^,\)]*), ([^,\)]*), ([^,\)]*)\)/\1, \2, \3, \4/g;" \
    -pe "s/Gdk.Rectangle//g;" \
    -pe "s/intersection = child_rect.intersect/intersects_, intersection = child_rect.intersect/g;" \
    -pe "s/event.state/event.get_state\(\)/g;" \
\
    -pe "s/import pango\n/from gi.repository import Pango\n/g;" \
    -pe "s/pango\./Pango\./g;" \
    -pe "s/Pango\.FontDescription/Pango\.Font\.description_from_string/g;" \
    -pe "s/Pango.ELLIPSIZE_/Pango.EllipsizeMode./g;" \
\
    -pe "s/import hippo\n/from gi.repository import Hippo\n/g;" \
    -pe "s/hippo\./Hippo\./g;" \
    -pe "s/Hippo\..*\.__init__/gobject.GObject.__init__/g;" \
    -pe "s/Hippo.PACK_/Hippo.PackFlags./g;" \
    -pe "s/Hippo.ORIENTATION_/Hippo.Orientation./g;" \
    -pe "#s/insert_sorted\(([^,\)]*), ([^,\)]*), ([^,\)]*)\)/insert_sorted\(\1, \2, \3, None\)/g;" \
    -pe "s/self\._box\.insert_sorted/#self\._box\.insert_sorted/g;" \
    -pe "s/self._box.append\(([^,\)]*)\)/self._box.append\(\1, 0\)/g;" \
    -pe "s/self.append\(self._buddy_icon\)/self.append\(self._buddy_icon, 0\)/g;" \
    -pe "s/self._box.sort\(([^,\)]*)\)/self._box.sort\(\1, None\)/g;" \
\
    -pe "s/import wnck\n/from gi.repository import Wnck\n/g;" \
    -pe "s/wnck\./Wnck\./g;" \
    -pe "s/Wnck.screen_get_default/Wnck.Screen.get_default/g;" \
    -pe "s/Wnck.WINDOW_/Wnck.WindowType./g;" \
\
    -pe "s/from sugar import _sugarext\n/from gi.repository import SugarExt\n/g;" \
    -pe "s/_sugarext\.ICON_ENTRY_/SugarExt.SexyIconEntryPosition./g;" \
    -pe "s/_sugarext\.IconEntry/SugarExt.SexyIconEntry/g;" \
    -pe "s/_sugarext\.SMClientXSMP/SugarExt.GsmClientXSMP/g;" \
    -pe "s/_sugarext\.VolumeAlsa/SugarExt.AcmeVolumeAlsa/g;" \
    -pe "s/_sugarext\./SugarExt\./g;" \
\
    -pe "s/import gtksourceview2\n/from gi.repository import GtkSource\n/g;" \
\
    -pe "#s/import cairo\n/from gi.repository import cairo\n/g;" \
\
    -pe "s/SugarExt.xsmp_init\(\)/'mec'/g;" \
    -pe "s/SugarExt.xsmp_run\(\)/#SugarExt.xsmp_run\(\)/g;" \
    -pe "s/SugarExt.session_create_global\(\)/None #SugarExt.session_create_global\(\)/g;" \
    -pe "s/self.session.start\(\)/return #self.session.start\(\)/g;" \
\
    -pe "s/self._box.sort\(self._layout.compare_activities, None\)/pass #self._box.sort(self._layout.compare_activities, None)/g;" \
    -pe "s/attach_points = info.get_attach_points/has_attach_points_, attach_points = info.get_attach_points/g;" \
    -pe "s/attach_points\[0\]\[0\]/attach_points\[0\].x/g;" \
    -pe "s/attach_points\[0\]\[1\]/attach_points\[0\].y/g;" \
    -pe "s/has_attach_points_/return 0,0;has_attach_points_/g;" \
    -pe "s/gobject.GObject.__init__\(self, self._model_filter\)/gobject.GObject.__init__\(self, model=self._model_filter\)/g;" \
    -pe "s/self._model_filter.set_visible_func/return;self._model_filter.set_visible_func/g;" \
    -pe "s/buddies_column.set_cell_data_func/return;buddies_column.set_cell_data_func/g;" \
    -pe "s/ column.set_cell_data_func/# column.set_cell_data_func/g;" \
    -pe "s/Hippo\.cairo_surface_from_gdk_pixbuf/SugarExt\.cairo_surface_from_pixbuf/g;" \
    $f
done

NEED_GOBJECT=`grep -R -l gobject\. $FILES_TO_CONVERT | xargs grep -nL import\ gobject`
for f in $NEED_GOBJECT; do
    sed -i "/import Gdk/ i\import gobject" $f
done

NEED_GOBJECT=`grep -R -l gobject\. $FILES_TO_CONVERT | xargs grep -nL import\ gobject`
for f in $NEED_GOBJECT; do
    sed -i "/import Gtk/ i\import gobject" $f
done

NEED_GOBJECT=`grep -R -l gobject\. $FILES_TO_CONVERT | xargs grep -nL import\ gobject`
for f in $NEED_GOBJECT; do
    sed -i "/import Hippo/ i\import gobject" $f
done

NEED_GDK=`grep -R -l Gdk\. $FILES_TO_CONVERT | xargs grep -nL import\ Gdk`
for f in $NEED_GDK; do
    sed -i "/import Gtk/ i\from gi.repository import Gdk" $f
done

NEED_GDK_X11=`grep -R -l GdkX11\. $FILES_TO_CONVERT | xargs grep -nL import\ GdkX11`
for f in $NEED_GDK_X11; do
    sed -i "/import Gdk/ i\from gi.repository import GdkX11" $f
done

NEED_SUGAR_EXT=`grep -R -l SugarExt\. $FILES_TO_CONVERT | xargs grep -nL import\ SugarExt`
for f in $NEED_SUGAR_EXT; do
    sed -i "/import cairo/ i\from gi.repository import SugarExt" $f
done

for f in sugar-pygi/src/jarabe/util/emulator.py sugar-pygi/bin/sugar-session; do
    sed -i "/import Gdk/ a\Gdk.init_check([])" $f
    sed -i "/import Gtk/ a\Gtk.init_check([])" $f
done

sed -i "/Gdk.threads_init()/ i\gobject.threads_init()" sugar-pygi/bin/sugar-session

# Disable treeview stuff
sed -i 's/class CellRendererIcon(Gtk.GenericCellRenderer):/class CellRendererIcon(Gtk.CellRenderer):/g' sugar-toolkit-pygi/src/sugar/graphics/icon.py

#sed -i '/def get_icon_state(base_name, perc, step=5):/ i\"""' sugar-toolkit-pygi/src/sugar/graphics/icon.py

#sed -i 's/from sugar.graphics.icon import Icon, CellRendererIcon/from sugar.graphics.icon import Icon/g' sugar-pygi/src/jarabe/desktop/activitieslist.py

#sed -i '/class CellRendererFavorite(CellRendererIcon):/ i\"""' sugar-pygi/src/jarabe/desktop/activitieslist.py
#sed -i '/def get_icon_state(base_name, perc, step=5):/ i\"""' sugar-toolkit-pygi/src/sugar/graphics/icon.py


