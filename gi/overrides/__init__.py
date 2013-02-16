import types
import warnings
import functools

from gi import PyGIDeprecationWarning
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
            raise KeyError('You have tried to modify the registry outside of the overrides module.  This is not allowed')

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
        if not hasattr(func, '__info__'):
            raise TypeError("func must be an gi function")
        from ..importer import modules
        module_name = func.__module__.rsplit('.', 1)[-1]
        self.module = modules[module_name]._introspection_module

    def __call__(self, func):
        def wrapper(*args, **kwargs):
            return func(*args, **kwargs)
        wrapper.__name__ = func.__name__
        setattr(self.module, func.__name__, wrapper)
        return wrapper

registry = _Registry()


def override(type_):
    '''Decorator for registering an override'''
    if isinstance(type_, types.FunctionType):
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
