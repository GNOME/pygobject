#!/usr/bin/env python

title = "File List (GenericTreeModel)"
description = """
This is a file list demo which makes use of the GenericTreeModel python
implementation of the Gtk.TreeModel interface. This demo shows what methods
need to be overridden to provide a valid TreeModel to a TreeView.
"""

import os
import stat
import time

import pygtkcompat
pygtkcompat.enable()
pygtkcompat.enable_gtk('3.0')

import gtk


folderxpm = [
    "17 16 7 1",
    "  c #000000",
    ". c #808000",
    "X c yellow",
    "o c #808080",
    "O c #c0c0c0",
    "+ c white",
    "@ c None",
    "@@@@@@@@@@@@@@@@@",
    "@@@@@@@@@@@@@@@@@",
    "@@+XXXX.@@@@@@@@@",
    "@+OOOOOO.@@@@@@@@",
    "@+OXOXOXOXOXOXO. ",
    "@+XOXOXOXOXOXOX. ",
    "@+OXOXOXOXOXOXO. ",
    "@+XOXOXOXOXOXOX. ",
    "@+OXOXOXOXOXOXO. ",
    "@+XOXOXOXOXOXOX. ",
    "@+OXOXOXOXOXOXO. ",
    "@+XOXOXOXOXOXOX. ",
    "@+OOOOOOOOOOOOO. ",
    "@                ",
    "@@@@@@@@@@@@@@@@@",
    "@@@@@@@@@@@@@@@@@"
    ]
folderpb = gtk.gdk.pixbuf_new_from_xpm_data(folderxpm)

filexpm = [
    "12 12 3 1",
    "  c #000000",
    ". c #ffff04",
    "X c #b2c0dc",
    "X        XXX",
    "X ...... XXX",
    "X ......   X",
    "X .    ... X",
    "X ........ X",
    "X .   .... X",
    "X ........ X",
    "X .     .. X",
    "X ........ X",
    "X .     .. X",
    "X ........ X",
    "X          X"
    ]
filepb = gtk.gdk.pixbuf_new_from_xpm_data(filexpm)


class FileListModel(gtk.GenericTreeModel):
    __gtype_name__ = 'DemoFileListModel'

    column_types = (gtk.gdk.Pixbuf, str, int, str, str)
    column_names = ['Name', 'Size', 'Mode', 'Last Changed']

    def __init__(self, dname=None):
        gtk.GenericTreeModel.__init__(self)
        self._sort_column_id = 0
        self._sort_order = gtk.SORT_ASCENDING

        if not dname:
            self.dirname = os.path.expanduser('~')
        else:
            self.dirname = os.path.abspath(dname)
        self.files = ['..'] + [f for f in os.listdir(self.dirname)]
        return

    def get_pathname(self, path):
        filename = self.files[path[0]]
        return os.path.join(self.dirname, filename)

    def is_folder(self, path):
        filename = self.files[path[0]]
        pathname = os.path.join(self.dirname, filename)
        filestat = os.stat(pathname)
        if stat.S_ISDIR(filestat.st_mode):
            return True
        return False

    def get_column_names(self):
        return self.column_names[:]

    #
    # GenericTreeModel Implementation
    #
    def on_get_flags(self):
        return 0  # gtk.TREE_MODEL_ITERS_PERSIST

    def on_get_n_columns(self):
        return len(self.column_types)

    def on_get_column_type(self, n):
        return self.column_types[n]

    def on_get_iter(self, path):
        return self.files[path[0]]

    def on_get_path(self, rowref):
        return self.files.index(rowref)

    def on_get_value(self, rowref, column):
        fname = os.path.join(self.dirname, rowref)
        try:
            filestat = os.stat(fname)
        except OSError:
            return None
        mode = filestat.st_mode
        if column is 0:
            if stat.S_ISDIR(mode):
                return folderpb
            else:
                return filepb
        elif column is 1:
            return rowref
        elif column is 2:
            return filestat.st_size
        elif column is 3:
            return oct(stat.S_IMODE(mode))
        return time.ctime(filestat.st_mtime)

    def on_iter_next(self, rowref):
        try:
            i = self.files.index(rowref) + 1
            return self.files[i]
        except IndexError:
            return None

    def on_iter_children(self, rowref):
        if rowref:
            return None
        return self.files[0]

    def on_iter_has_child(self, rowref):
        return False

    def on_iter_n_children(self, rowref):
        if rowref:
            return 0
        return len(self.files)

    def on_iter_nth_child(self, rowref, n):
        if rowref:
            return None
        try:
            return self.files[n]
        except IndexError:
            return None

    def on_iter_parent(child):
        return None


class GenericTreeModelExample:
    def delete_event(self, widget, event, data=None):
        gtk.main_quit()
        return False

    def __init__(self):
        # Create a new window
        self.window = gtk.Window(type=gtk.WINDOW_TOPLEVEL)

        self.window.set_size_request(300, 200)

        self.window.connect("delete_event", self.delete_event)

        self.listmodel = FileListModel()

        # create the TreeView
        self.treeview = gtk.TreeView()

        self.tvcolumns = []

        # create the TreeViewColumns to display the data
        for n, name in enumerate(self.listmodel.get_column_names()):
            if n == 0:
                cellpb = gtk.CellRendererPixbuf()
                col = gtk.TreeViewColumn(name, cellpb, pixbuf=0)
                cell = gtk.CellRendererText()
                col.pack_start(cell, False)
                col.add_attribute(cell, 'text', 1)
            else:
                cell = gtk.CellRendererText()
                col = gtk.TreeViewColumn(name, cell, text=n + 1)
            if n == 1:
                cell.set_property('xalign', 1.0)

            self.treeview.append_column(col)

        self.treeview.connect('row-activated', self.open_file)

        self.scrolledwindow = gtk.ScrolledWindow()
        self.scrolledwindow.add(self.treeview)
        self.window.add(self.scrolledwindow)
        self.treeview.set_model(self.listmodel)
        self.window.set_title(self.listmodel.dirname)
        self.window.show_all()

    def open_file(self, treeview, path, column):
        model = treeview.get_model()
        if model.is_folder(path):
            pathname = model.get_pathname(path)
            new_model = FileListModel(pathname)
            self.window.set_title(new_model.dirname)
            treeview.set_model(new_model)
        return


def main(demoapp=None):
    demo = GenericTreeModelExample()
    demo
    gtk.main()

if __name__ == "__main__":
    main()
