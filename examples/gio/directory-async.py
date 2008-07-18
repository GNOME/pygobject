import gobject
import gio

def next_files_done(enumerator, result):
    print 'done!'
    for file_info in enumerator.next_files_finish(result):
        print file_info.get_name()
    loop.quit()

def enumerate_children_done(gfile, result):
    enumerator = gfile.enumerate_children_finish(result)
    enumerator.next_files_async(10, next_files_done)

gfile = gio.File("/")
gfile.enumerate_children_async(
    "standard::name", enumerate_children_done)

loop = gobject.MainLoop()
loop.run()
