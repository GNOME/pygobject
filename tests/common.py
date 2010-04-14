import os
import sys

def importModules(buildDir, srcDir):
    # Be very careful when you change this code, it's
    # fragile and the order is really significant

    sys.path.insert(0, srcDir)
    sys.path.insert(0, buildDir)
    sys.path.insert(0, os.path.join(buildDir, 'glib'))
    sys.path.insert(0, os.path.join(buildDir, 'gobject'))
    sys.path.insert(0, os.path.join(buildDir, 'gio'))

    # testhelper
    sys.path.insert(0, os.path.join(buildDir, 'tests'))
    sys.argv.append('--g-fatal-warnings')

    testhelper = importModule('testhelper', '.')
    glib = importModule('glib', buildDir, 'glib')
    gobject = importModule('gobject', buildDir, 'gobject')
    gio = importModule('gio', buildDir, 'gio')

    globals().update(locals())

    os.environ['PYGTK_USE_GIL_STATE_API'] = ''
    gobject.threads_init()

def importModule(module, directory, name=None):
    global isDistCheck

    origName = module
    if not name:
        name = module + '.la'

    try:
        obj = __import__(module, {}, {}, '')
    except ImportError:
        raise
        e = sys.exc_info()[1]
        raise SystemExit('%s could not be imported: %s' % (origName, e))

    location = obj.__file__

    current = os.getcwd()
    expected = os.path.abspath(os.path.join(current, location))
    current = os.path.abspath(location)
    if current != expected:
        raise AssertionError('module %s imported from wrong location. Expected %s, got %s' % (
                                 module, expected, current))
    return obj
