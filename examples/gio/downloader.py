# Example GIO based Asynchronous downloader

import sys

import glib
import gio


class Downloader(object):
    def __init__(self, uri):
        self.fetched = 0
        self.total = -1
        self.uri = uri
        self.loop = None
        self.gfile = gio.File(self.uri)
        self.output = self.get_output_filename()
        self.fd = None

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
        try:
            stream = gfile.read_finish(result)
        except gio.Error, e:
            print 'ERROR: %s' % (e.message,)
            self.stop()
            return
        info = gfile.query_info('standard::size')
        self.total = info.get_attribute_uint64('standard::size')
        stream.read_async(4096, self.stream_read_callback)

    def data_read(self, data):
        if self.fd is None:
            self.fd = open(self.output, 'w')
        self.fd.write(data)
        self.fetched += len(data)
        if self.total != -1:
            print '%7.2f %%' % (self.fetched / float(self.total) * 100)

    def data_finished(self):
        print '%d bytes read.' % (self.fetched,)
        self.fd.close()
        self.stop()

    def start(self):
        print 'Downloading %s -> %s' % (self.uri, self.output)
        self.gfile.read_async(self.read_callback)
        self.loop = glib.MainLoop()
        self.loop.run()

    def stop(self):
        self.loop.quit()


def main(args):
    if len(args) < 2:
        print 'Needs a URI'
        return 1

    d = Downloader(args[1])
    d.start()

if __name__ == '__main__':
    sys.exit(main(sys.argv))
