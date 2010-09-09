import sys

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

    '''
    for tests that need to write to intefaces that take bytes in
    python 3

    python 3 has a seperate bytes type for low level functions like os.write

    python 2 treats these as strings

    any tests that need to write a string of bytes should do something like
    this:

    from compathelper import _bytes
    os.write(_bytes("hello"))
    '''

    _bytes = lambda s: s.encode()
else:
    _long = long
    _basestring = basestring
    _bytes = str
