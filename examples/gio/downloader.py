# Example GIO based Asynchronous downloader

import sys

import glib
import glib.option
import gio


class Downloader(object):
    def __init__(self, uri):
        self.total = 0
        self.gfile = gio.File(uri)
        self.loop = glib.MainLoop()

        output = self.get_output_filename()
        self.fd = open(output, 'w')
        print 'Downloading %s -> %s' % (uri, output)

        self.gfile.read_async(self.read_callback)

    def get_output_filename(self):
        basename = self.gfile.get_basename()
        if basename == '/':
            basename = 'index.html'
        return basename

    def stream_read_callback(self, stream, result):
        data = stream.read_finish(result)
        if not data:
            self.data_finished()
            return
        self.data_read(data)
        stream.read_async(4096, self.stream_read_callback)


    def read_callback(self, gfile, result):
        stream = gfile.read_finish(result)
        stream.read_async(4096, self.stream_read_callback)

    def data_read(self, data):
        self.fd.write(data)
        self.total += len(data)

    def data_finished(self):
        print '%d bytes read' % (self.total,)
        self.loop.quit()

    def run(self):
        self.loop.run()

def main(args):
    if len(args) < 2:
        print 'Needs a URI'
        return 1

    d = Downloader(args[1])
    d.run()

if __name__ == '__main__':
    sys.exit(main(sys.argv))
