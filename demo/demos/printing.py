#!/usr/bin/env python
# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab
#
# Copyright (C) 2010 Red Hat, Inc., John (J5) Palmieri <johnp@redhat.com>
#
# This library is free software; you can redistribute it and/or
# modify it under the terms of the GNU Lesser General Public
# License as published by the Free Software Foundation; either
# version 2.1 of the License, or (at your option) any later version.
#
# This library is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
# Lesser General Public License for more details.
#
# You should have received a copy of the GNU Lesser General Public
# License along with this library; if not, write to the Free Software
# Foundation, Inc., 51 Franklin St, Fifth Floor, Boston, MA  02110-1301
# USA

title = "Printing"
description = """
GtkPrintOperation offers a simple API to support printing in a cross-platform way.
"""

from gi.repository import Gtk, GLib, Pango, PangoCairo
import math
import os


class PrintingApp:
    HEADER_HEIGHT = 10 * 72 / 25.4
    HEADER_GAP = 3 * 72 / 25.4

    def __init__(self):
        self.operation = Gtk.PrintOperation()
        print_data = {'filename': os.path.abspath(__file__),
                      'font_size': 12.0,
                      'lines_per_page': 0,
                      'lines': None,
                      'num_lines': 0,
                      'num_pages': 0
                     }

        self.operation.connect('begin-print', self.begin_print, print_data)
        self.operation.connect('draw-page', self.draw_page, print_data)
        self.operation.connect('end-print', self.end_print, print_data)

        self.operation.set_use_full_page(False)
        self.operation.set_unit(Gtk.Unit.POINTS)
        self.operation.set_embed_page_setup(True)

        settings = Gtk.PrintSettings()

        dir = GLib.get_user_special_dir(GLib.UserDirectory.DIRECTORY_DOCUMENTS)
        if dir is None:
            dir = GLib.get_home_dir()
        if settings.get(Gtk.PRINT_SETTINGS_OUTPUT_FILE_FORMAT) == 'ps':
            ext = '.ps'
        elif settings.get(Gtk.PRINT_SETTINGS_OUTPUT_FILE_FORMAT) == 'svg':
            ext = '.svg'
        else:
            ext = '.pdf'

        uri = "file://%s/gtk-demo%s" % (dir, ext)
        settings.set(Gtk.PRINT_SETTINGS_OUTPUT_URI, uri)
        self.operation.set_print_settings(settings)

    def run(self, parent=None):
        result = self.operation.run(Gtk.PrintOperationAction.PRINT_DIALOG, parent)

        if result == Gtk.PrintOperationResult.ERROR:
            message = self.operation.get_error()

            dialog = Gtk.MessageDialog(parent,
                                       0,
                                       Gtk.MessageType.ERROR,
                                       Gtk.ButtonsType.CLOSE,
                                       message)

            dialog.run()
            dialog.destroy()

        Gtk.main_quit()

    def begin_print(self, operation, print_ctx, print_data):
        height = print_ctx.get_height() - self.HEADER_HEIGHT - self.HEADER_GAP
        print_data['lines_per_page'] = \
            math.floor(height / print_data['font_size'])

        file_path = print_data['filename']
        if not os.path.isfile(file_path):
            file_path = os.path.join('demos', file_path)
            if not os.path.isfile:
                raise Exception("file not found: " % (file_path, ))

        # in reality you should most likely not read the entire
        # file into a buffer
        source_file = open(file_path, 'r')
        s = source_file.read()
        print_data['lines'] = s.split('\n')

        print_data['num_lines'] = len(print_data['lines'])
        print_data['num_pages'] = \
            (print_data['num_lines'] - 1) / print_data['lines_per_page'] + 1

        operation.set_n_pages(print_data['num_pages'])

    def draw_page(self, operation, print_ctx, page_num, print_data):
        cr = print_ctx.get_cairo_context()
        width = print_ctx.get_width()

        cr.rectangle(0, 0, width, self.HEADER_HEIGHT)
        cr.set_source_rgb(0.8, 0.8, 0.8)
        cr.fill_preserve()

        cr.set_source_rgb(0, 0, 0)
        cr.set_line_width(1)
        cr.stroke()

        layout = print_ctx.create_pango_layout()
        desc = Pango.FontDescription('sans 14')
        layout.set_font_description(desc)

        layout.set_text(print_data['filename'], -1)
        (text_width, text_height) = layout.get_pixel_size()

        if text_width > width:
            layout.set_width(width)
            layout.set_ellipsize(Pango.EllipsizeMode.START)
            (text_width, text_height) = layout.get_pixel_size()

        cr.move_to((width - text_width) / 2,
                   (self.HEADER_HEIGHT - text_height) / 2)
        PangoCairo.show_layout(cr, layout)

        page_str = "%d/%d" % (page_num + 1, print_data['num_pages'])
        layout.set_text(page_str, -1)

        layout.set_width(-1)
        (text_width, text_height) = layout.get_pixel_size()
        cr.move_to(width - text_width - 4,
                   (self.HEADER_HEIGHT - text_height) / 2)
        PangoCairo.show_layout(cr, layout)

        layout = print_ctx.create_pango_layout()

        desc = Pango.FontDescription('monospace')
        desc.set_size(print_data['font_size'] * Pango.SCALE)
        layout.set_font_description(desc)

        cr.move_to(0, self.HEADER_HEIGHT + self.HEADER_GAP)
        lines_pp = int(print_data['lines_per_page'])
        num_lines = print_data['num_lines']
        data_lines = print_data['lines']
        font_size = print_data['font_size']
        line = page_num * lines_pp

        for i in range(lines_pp):
            if line >= num_lines:
                break

            layout.set_text(data_lines[line], -1)
            PangoCairo.show_layout(cr, layout)
            cr.rel_move_to(0, font_size)
            line += 1

    def end_print(self, operation, print_ctx, print_data):
        pass


def main(demoapp=None):
    app = PrintingApp()
    GLib.idle_add(app.run, demoapp.window)
    Gtk.main()

if __name__ == '__main__':
    main()
