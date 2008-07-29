# Async Directory listing
# Johan Dahlin 2008
import sys

import glib
import gio

def next_files_done(enumerator, result):
    for file_info in enumerator.next_files_finish(result):
        print file_info.get_name()
    loop.quit()

def enumerate_children_done(gfile, result):
    try:
        enumerator = gfile.enumerate_children_finish(result)
    except gio.Error, e:
        print 'ERROR:', e
        loop.quit()
        return
    enumerator.next_files_async(100, next_files_done)

if len(sys.argv) >= 2:
    uri = sys.argv[1]
else:
    uri = "/"

gfile = gio.File(uri)
gfile.enumerate_children_async(
    "standard::name", enumerate_children_done)

loop = glib.MainLoop()
loop.run()
