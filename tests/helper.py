import contextlib
import unittest
import inspect
import warnings
import functools
import sys
from collections import namedtuple
from io import StringIO

import gi
from gi import PyGIDeprecationWarning
from gi.repository import GLib


ExceptionInfo = namedtuple("ExceptionInfo", ["type", "value", "traceback"])
"""The type used for storing exceptions used by capture_exceptions()"""


@contextlib.contextmanager
def capture_exceptions():
    """Installs a temporary sys.excepthook which records all exceptions
    instead of printing them.
    """
    exceptions = []

    def custom_excepthook(*args):
        exceptions.append(ExceptionInfo(*args))

    old_hook = sys.excepthook
    sys.excepthook = custom_excepthook
    try:
        yield exceptions
    finally:
        sys.excepthook = old_hook


def ignore_gi_deprecation_warnings(func_or_class):
    """A unittest class and function decorator which makes them ignore
    PyGIDeprecationWarning.
    """
    if inspect.isclass(func_or_class):
        assert issubclass(func_or_class, unittest.TestCase)
        cls = func_or_class
        for name, value in cls.__dict__.items():
            if callable(value) and name.startswith("test_"):
                new_value = ignore_gi_deprecation_warnings(value)
                setattr(cls, name, new_value)
        return cls
    func = func_or_class

    @functools.wraps(func)
    def wrapper(*args, **kwargs):
        with capture_gi_deprecation_warnings():
            return func(*args, **kwargs)

    return wrapper


@contextlib.contextmanager
def capture_gi_deprecation_warnings():
    """Temporarily suppress PyGIDeprecationWarning output and record them."""
    with warnings.catch_warnings(record=True) as warn:
        warnings.simplefilter("always", category=PyGIDeprecationWarning)
        yield warn


@contextlib.contextmanager
def capture_glib_warnings(allow_warnings=False, allow_criticals=False):
    """Temporarily suppress glib warning output and record them.

    The test suite is run with G_DEBUG="fatal-warnings fatal-criticals"
    by default. Setting allow_warnings and allow_criticals will temporarily
    allow warnings or criticals without terminating the test run.
    """
    old_mask = GLib.log_set_always_fatal(GLib.LogLevelFlags(0))

    new_mask = old_mask
    if allow_warnings:
        new_mask &= ~GLib.LogLevelFlags.LEVEL_WARNING
    if allow_criticals:
        new_mask &= ~GLib.LogLevelFlags.LEVEL_CRITICAL

    GLib.log_set_always_fatal(GLib.LogLevelFlags(new_mask))

    GLibWarning = gi._gi.Warning
    try:
        with warnings.catch_warnings(record=True) as warn:
            warnings.filterwarnings("always", category=GLibWarning)
            yield warn
    finally:
        GLib.log_set_always_fatal(old_mask)


@contextlib.contextmanager
def capture_glib_deprecation_warnings():
    """Temporarily suppress glib deprecation warning output and record them."""
    GLibWarning = gi._gi.Warning
    with warnings.catch_warnings(record=True) as warn:
        warnings.filterwarnings(
            "always",
            category=GLibWarning,
            message=".+ is deprecated and shouldn't be used anymore\\. "
            "It will be removed in a future version\\.",
        )
        yield warn


@contextlib.contextmanager
def capture_output():
    """With capture_output() as (stdout, stderr):
        some_action()
    print(stdout.getvalue(), stderr.getvalue()).
    """
    err = StringIO()
    out = StringIO()
    old_err = sys.stderr
    old_out = sys.stdout
    sys.stderr = err
    sys.stdout = out

    try:
        yield (out, err)
    finally:
        sys.stderr = old_err
        sys.stdout = old_out
