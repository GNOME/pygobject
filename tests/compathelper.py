import sys

PY2 = PY3 = False

if sys.version_info >= (3, 0):
    '''
    for tests that need to test long values in python 2

    python 3 does not differentiate between long and int
    and does not supply a long keyword

    instead of testing longs by using values such as 10L
    test writters should do this:

    from compathelper import _long
    _long(10)
    '''
    _long = int

    '''
    for tests that need to test string values in python 2

    python 3 does differentiate between str and bytes
    and does not supply a basestring keyword

    any tests that use basestring should do this:

    from compathelper import _basestring
    isinstance(_basestring, "hello")
    '''
    _basestring = str

    from io import StringIO
    StringIO
    PY3 = True

    def reraise(tp, value, tb):
        raise tp(value).with_traceback(tb)
else:
    _long = long
    _basestring = basestring
    from StringIO import StringIO
    StringIO
    PY2 = True

    exec("def reraise(tp, value, tb):\n raise tp, value, tb")
