import types
import warnings

from gi import PyGIDeprecationWarning
from gi._gi import CallableInfo
from gi._constants import \
    TYPE_NONE, \
    TYPE_INVALID

# support overrides in different directories than our gi module
from pkgutil import extend_path
__path__ = extend_path(__path__, __name__)

registry = None


def wraps(wrapped):
    def assign(wrapper):
        wrapper.__name__ = wrapped.__name__
        wrapper.__module__ = wrapped.__module__
        return wrapper
    return assign


class _Registry(dict):
    def __setitem__(self, key, value):
        """We do checks here to make sure only submodules of the override
        module are added.  Key and value should be the same object and come
        from the gi.override module.

        We add the override to the dict as "override_module.name".  For instance
        if we were overriding Gtk.Button you would retrive it as such:
        registry['Gtk.Button']
        """
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
    """decorator for overriding a function"""
    def __init__(self, func):
        if not isinstance(func, CallableInfo):
            raise TypeError("func must be a gi function, got %s" % func)
        from ..importer import modules
        module_name = func.__module__.rsplit('.', 1)[-1]
        self.module = modules[module_name]._introspection_module

    def __call__(self, func):
        setattr(self.module, func.__name__, func)
        return func

registry = _Registry()


def override(type_):
    """Decorator for registering an override"""
    if isinstance(type_, (types.FunctionType, CallableInfo)):
        return overridefunc(type_)
    else:
        registry.register(type_)
        return type_


def deprecated(fn, replacement):
    """Decorator for marking methods and classes as deprecated"""
    @wraps(fn)
    def wrapped(*args, **kwargs):
        warnings.warn('%s is deprecated; use %s instead' % (fn.__name__, replacement),
                      PyGIDeprecationWarning, stacklevel=2)
        return fn(*args, **kwargs)
    return wrapped


def deprecated_init(super_init_func, arg_names, ignore=tuple(),
                    deprecated_aliases={}, deprecated_defaults={},
                    category=PyGIDeprecationWarning,
                    stacklevel=2):
    """Wrapper for deprecating GObject based __init__ methods which specify
    defaults already available or non-standard defaults.

    :param callable super_init_func:
        Initializer to wrap.
    :param list arg_names:
        Ordered argument name list.
    :param list ignore:
        List of argument names to ignore when calling the wrapped function.
        This is useful for function which take a non-standard keyword that is munged elsewhere.
    :param dict deprecated_aliases:
        Dictionary mapping a keyword alias to the actual g_object_newv keyword.
    :param dict deprecated_defaults:
        Dictionary of non-standard defaults that will be used when the
        keyword is not explicitly passed.
    :param Exception category:
        Exception category of the error.
    :param int stacklevel:
        Stack level for the deprecation passed on to warnings.warn
    :returns: Wrapped version of ``super_init_func`` which gives a deprecation
        warning when non-keyword args or aliases are used.
    :rtype: callable
    """
    # We use a list of argument names to maintain order of the arguments
    # being deprecated. This allows calls with positional arguments to
    # continue working but with a deprecation message.
    def new_init(self, *args, **kwargs):
        """Initializer for a GObject based classes with support for property
        sets through the use of explicit keyword arguments.
        """
        # Print warnings for calls with positional arguments.
        if args:
            warnings.warn('Using positional arguments with the GObject constructor has been deprecated. '
                          'Please specify keyword(s) for "%s" or use a class specific constructor. '
                          'See: https://wiki.gnome.org/PyGObject/InitializerDeprecations' %
                          ', '.join(arg_names[:len(args)]),
                          category, stacklevel=stacklevel)
            new_kwargs = dict(zip(arg_names, args))
        else:
            new_kwargs = {}
        new_kwargs.update(kwargs)

        # Print warnings for alias usage and transfer them into the new key.
        aliases_used = []
        for key, alias in deprecated_aliases.items():
            if alias in new_kwargs:
                new_kwargs[key] = new_kwargs.pop(alias)
                aliases_used.append(key)

        if aliases_used:
            warnings.warn('The keyword(s) "%s" have been deprecated in favor of "%s" respectively. '
                          'See: https://wiki.gnome.org/PyGObject/InitializerDeprecations' %
                          (', '.join(deprecated_aliases[k] for k in sorted(aliases_used)),
                           ', '.join(sorted(aliases_used))),
                          category, stacklevel=stacklevel)

        # Print warnings for defaults different than what is already provided by the property
        defaults_used = []
        for key, value in deprecated_defaults.items():
            if key not in new_kwargs:
                new_kwargs[key] = deprecated_defaults[key]
                defaults_used.append(key)

        if defaults_used:
            warnings.warn('Initializer is relying on deprecated non-standard '
                          'defaults. Please update to explicitly use: %s '
                          'See: https://wiki.gnome.org/PyGObject/InitializerDeprecations' %
                          ', '.join('%s=%s' % (k, deprecated_defaults[k]) for k in sorted(defaults_used)),
                          category, stacklevel=stacklevel)

        # Remove keywords that should be ignored.
        for key in ignore:
            if key in new_kwargs:
                new_kwargs.pop(key)

        return super_init_func(self, **new_kwargs)

    return new_init


def strip_boolean_result(method, exc_type=None, exc_str=None, fail_ret=None):
    """Translate method's return value for stripping off success flag.

    There are a lot of methods which return a "success" boolean and have
    several out arguments. Translate such a method to return the out arguments
    on success and None on failure.
    """
    @wraps(method)
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
