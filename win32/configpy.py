# Find out configuration of Python installation
import sys
import platform

def configpy():
    confignmake = open('configpy.mak', 'w')
    pyver = platform.python_version().split('.')
    compiler_details = platform.python_compiler().split()
    compiler_ver = compiler_details[1][2:compiler_details[1].find('00')]

    confignmake.write('PYTHONPREFIX=%s\n' % sys.prefix)
    confignmake.write('PYTHONMAJ=%s\n' % pyver[0])
    confignmake.write('PYTHONSERIESDOT=%s.%s\n' % (pyver[0], pyver[1]))
    confignmake.write('PYTHONCOMPILER=%s\n' % compiler_details[0])
    confignmake.write('PYTHONCOMPILERVER=%s\n' % compiler_ver)
    try:
        import cairo
        confignmake.write('PYCAIRO=1')
    except ImportError:
        confignmake.write('PYCAIRO=')
    confignmake.close()

if __name__ == '__main__':
    configpy()