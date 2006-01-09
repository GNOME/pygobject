import os
import sys

def importModules(buildDir, srcDir):
    # Be very careful when you change this code, it's
    # fragile and the order is really significant
    
    # ltihooks
    sys.path.insert(0, srcDir)
    import ltihooks
    sys.path.remove(srcDir)

    # gobject
    sys.path.insert(0, os.path.join(buildDir, 'gobject'))
    # testhelper
    sys.path.insert(0, os.path.join(buildDir, 'tests'))
    sys.argv.append('--g-fatal-warnings')
    
    gobject = importModule('gobject', buildDir, 'gobject/gobject.la')
    testhelper = importModule('testhelper', '.')
        
    ltihooks.uninstall()
    del ltihooks
    
    globals().update(locals())
    
    os.environ['PYGTK_USE_GIL_STATE_API'] = ''
    gobject.threads_init()

def importModule(module, directory, name=None):
    global isDistCheck

    origName = module
    if '.' in module:
        fromlist = '.'.join(module.split('.')[:-1])
    else:
        fromlist = None

    if not name:
        name = module + '.la'

    try:
        obj = __import__(module, {}, {}, fromlist)
    except ImportError, e:
        print 'WARNING: %s could not be imported' % origName
        print e
        return 
    
    if hasattr(obj, '__file__'):
        location = obj.__file__
    else:
        package = __import__(fromlist)
        location = os.path.join(package.__file__, name)

    current = os.getcwd()
    expected = os.path.abspath(os.path.join(current, location))
    current = os.path.abspath(location)
    if current != expected:
        raise AssertionError('module %s imported from wrong location. Expected %s, got %s' % (
	                         module, expected, current))

    return obj
    
