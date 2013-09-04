import types
import warnings
import functools

from gi import PyGIDeprecationWarning
from gi._gi import CallableInfo
from gi._gobject.constants import \
    TYPE_NONE, \
    TYPE_INVALID

# support overrides in different directories than our gi module
from pkgutil import extend_path
__path__ = extend_path(__path__, __name__)

registry = None


class _Registry(dict):
    def __setitem__(self, key, value):
        '''We do checks here to make sure only submodules of the override
        module are added.  Key and value should be the same object and come
        from the gi.override module.

        We add the override to the dict as "override_module.name".  For instance
        if we were overriding Gtk.Button you would retrive it as such:
        registry['Gtk.Button']
        '''
        if not key == value:
            raise KeyError('You have tried to modify the registry.  This should only be done by the override decorator')

        try:
            info = getattr(value, '__info__')
        except AttributeError:
            raise TypeError('Can not override a type %s, which is not in a gobject introspection typelib' % value.__name__)

        if not value.__module__.startswith('gi.overrides'):
            raise KeyError('You have tried to modify the registry outside of the overrides module. '
                           'This is not allowed (%s, %s)' % (value, value.__module__))

        g_type = info.get_g_type()
        assert g_type != TYPE_NONE
        if g_type != TYPE_INVALID:
            g_type.pytype = value

            # strip gi.overrides from module name
            module = value.__module__[13:]
            key = "%s.%s" % (module, value.__name__)
            super(_Registry, self).__setitem__(key, value)

    def register(self, override_class):
        self[override_class] = override_class


class overridefunc(object):
    '''decorator for overriding a function'''
    def __init__(self, func):
        if not isinstance(func, CallableInfo):
            raise TypeError("func must be a gi function, got %s" % func)
        from ..importer import modules
        module_name = func.__module__.rsplit('.', 1)[-1]
        self.module = modules[module_name]._introspection_module

    def __call__(self, func):
        def wrapper(*args, **kwargs):
            return func(*args, **kwargs)
        wrapper.__name__ = func.__name__
        wrapper.__doc__ = func.__doc__
        setattr(self.module, func.__name__, wrapper)
        return wrapper

registry = _Registry()


def override(type_):
    '''Decorator for registering an override'''
    if isinstance(type_, (types.FunctionType, CallableInfo)):
        return overridefunc(type_)
    else:
        registry.register(type_)
        return type_


def deprecated(fn, replacement):
    '''Decorator for marking methods and classes as deprecated'''
    @functools.wraps(fn)
    def wrapped(*args, **kwargs):
        warnings.warn('%s is deprecated; use %s instead' % (fn.__name__, replacement),
                      PyGIDeprecationWarning, stacklevel=2)
        return fn(*args, **kwargs)
    return wrapped


def strip_boolean_result(method, exc_type=None, exc_str=None, fail_ret=None):
    '''Translate method's return value for stripping off success flag.

    There are a lot of methods which return a "success" boolean and have
    several out arguments. Translate such a method to return the out arguments
    on success and None on failure.
    '''
    @functools.wraps(method)
    def wrapped(*args, **kwargs):
        ret = method(*args, **kwargs)
        if ret[0]:
            if len(ret) == 2:
                return ret[1]
            else:
                return ret[1:]
        else:
            if exc_type:
                raise exc_type(exc_str or 'call failed')
            return fail_ret
    return wrapped
