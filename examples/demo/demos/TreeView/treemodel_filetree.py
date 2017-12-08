#!/usr/bin/env python

title = "File Tree (GenericTreeModel)"
description = """
This is a file list demo which makes use of the GenericTreeModel python
implementation of the Gtk.TreeModel interface. This demo shows what methods
need to be overridden to provide a valid TreeModel to a TreeView.
"""

import os
import stat
import time
from collections import OrderedDict

import pygtkcompat
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


class FileTreeModel(gtk.GenericTreeModel):
    __gtype_name__ = 'DemoFileTreeModel'

    column_types = (gtk.gdk.Pixbuf, str, int, str, str)
    column_names = ['Name', 'Size', 'Mode', 'Last Changed']

    def __init__(self, dname=None):
        gtk.GenericTreeModel.__init__(self)
        if not dname:
            self.dirname = os.path.expanduser('~')
        else:
            self.dirname = os.path.abspath(dname)
        self.files = self.build_file_dict(self.dirname)
        return

    def build_file_dict(self, dirname):
        """
        :Returns:
            A dictionary containing the files in the given dirname keyed by filename.
            If the child filename is a sub-directory, the dict value is a dict.
            Otherwise it will be None.
        """
        d = OrderedDict()
        for fname in os.listdir(dirname):
            try:
                filestat = os.stat(os.path.join(dirname, fname))
            except OSError:
                d[fname] = None
            else:
                d[fname] = OrderedDict() if stat.S_ISDIR(filestat.st_mode) else None

        return d

    def get_node_from_treepath(self, path):
        """
        :Returns:
            The node stored at the given tree path in local storage.
        """
        # TreePaths are a series of integer indices so just iterate through them
        # and index values by each integer since we are using an OrderedDict
        if path is None:
            path = []
        node = self.files
        for index in path:
            node = list(node.values())[index]
        return node

    def get_node_from_filepath(self, filepath):
        """
        :Returns:
            The node stored at the given file path in local storage.
        """
        if not filepath:
            return self.files
        node = self.files
        for key in filepath.split(os.path.sep):
            node = node[key]
        return node

    def get_column_names(self):
        return self.column_names[:]

    #
    # GenericTreeModel Implementation
    #

    def on_get_flags(self):
        return 0

    def on_get_n_columns(self):
        return len(self.column_types)

    def on_get_column_type(self, n):
        return self.column_types[n]

    def on_get_path(self, relpath):
        path = []
        node = self.files
        for key in relpath.split(os.path.sep):
            path.append(list(node.keys()).index(key))
            node = node[key]
        return path

    def on_get_value(self, relpath, column):
        fname = os.path.join(self.dirname, relpath)
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
            return os.path.basename(relpath)
        elif column is 2:
            return filestat.st_size
        elif column is 3:
            return oct(stat.S_IMODE(mode))
        return time.ctime(filestat.st_mtime)

    def on_get_iter(self, path):
        filepath = ''
        value = self.files
        for index in path:
            filepath = os.path.join(filepath, list(value.keys())[index])
            value = list(value.values())[index]
        return filepath

    def on_iter_next(self, filepath):
        parent_path, child_path = os.path.split(filepath)
        parent = self.get_node_from_filepath(parent_path)

        # Index of filepath within its parents child list
        sibling_names = list(parent.keys())
        index = sibling_names.index(child_path)
        try:
            return os.path.join(parent_path, sibling_names[index + 1])
        except IndexError:
            return None

    def on_iter_children(self, filepath):
        if filepath:
            children = list(self.get_node_from_filepath(filepath).keys())
            if children:
                return os.path.join(filepath, children[0])
        elif self.files:
            return list(self.files.keys())[0]

        return None

    def on_iter_has_child(self, filepath):
        return bool(self.get_node_from_filepath(filepath))

    def on_iter_n_children(self, filepath):
        return len(self.get_node_from_filepath(filepath))

    def on_iter_nth_child(self, filepath, n):
        try:
            child = list(self.get_node_from_filepath(filepath).keys())[n]
            if filepath:
                return os.path.join(filepath, child)
            else:
                return child
        except IndexError:
            return None

    def on_iter_parent(self, filepath):
        return os.path.dirname(filepath)

    def on_ref_node(self, filepath):
        value = self.get_node_from_filepath(filepath)
        if value is not None:
            value.update(self.build_file_dict(os.path.join(self.dirname, filepath)))

    def on_unref_node(self, filepath):
        pass


class GenericTreeModelExample:
    def delete_event(self, widget, event, data=None):
        gtk.main_quit()
        return False

    def __init__(self):
        # Create a new window
        self.window = gtk.Window(type=gtk.WINDOW_TOPLEVEL)
        self.window.set_size_request(300, 200)
        self.window.connect("delete_event", self.delete_event)

        self.listmodel = FileTreeModel()

        # create the TreeView
        self.treeview = gtk.TreeView()

        # create the TreeViewColumns to display the data
        column_names = self.listmodel.get_column_names()
        self.tvcolumn = [None] * len(column_names)
        cellpb = gtk.CellRendererPixbuf()
        self.tvcolumn[0] = gtk.TreeViewColumn(column_names[0],
                                              cellpb, pixbuf=0)
        cell = gtk.CellRendererText()
        self.tvcolumn[0].pack_start(cell, False)
        self.tvcolumn[0].add_attribute(cell, 'text', 1)
        self.treeview.append_column(self.tvcolumn[0])
        for n in range(1, len(column_names)):
            cell = gtk.CellRendererText()
            if n == 1:
                cell.set_property('xalign', 1.0)
            self.tvcolumn[n] = gtk.TreeViewColumn(column_names[n],
                                                  cell, text=n + 1)
            self.treeview.append_column(self.tvcolumn[n])

        self.scrolledwindow = gtk.ScrolledWindow()
        self.scrolledwindow.add(self.treeview)
        self.window.add(self.scrolledwindow)
        self.treeview.set_model(self.listmodel)
        self.window.set_title(self.listmodel.dirname)
        self.window.show_all()


def main(demoapp=None):
    demo = GenericTreeModelExample()
    demo
    gtk.main()


if __name__ == "__main__":
    main()
