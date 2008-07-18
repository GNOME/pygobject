import gobject
import gio

def callback(gfile, result):
    for file_info in gfile.enumerate_children_finish(result):
        print file_info.get_name()
    loop.quit()

gfile = gio.File("/")
gfile.enumerate_children_async("standard::*", callback)

loop = gobject.MainLoop()
loop.run()
