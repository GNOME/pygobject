import warnings

from gi import PyGIDeprecationWarning

warnings.warn(
    'gi.pygtkcompat is being deprecated in favor of using "pygtkcompat" directly.',
    PyGIDeprecationWarning,
)

from pygtkcompat import enable


__all__ = ["enable"]
