# -*- coding: utf-8 -*-

import sys

import pytest

from .compathelper import reraise


@pytest.hookimpl(hookwrapper=True)
def pytest_runtest_call(item):
    """A pytest hook which takes over sys.excepthook and raises any uncaught
    exception (with PyGObject this happesn often when we get called from C,
    like any signal handler, vfuncs tc)
    """

    assert sys.excepthook is sys.__excepthook__

    exceptions = []

    def on_hook(type_, value, tback):
        exceptions.append((type_, value, tback))

    sys.excepthook = on_hook
    try:
        yield
    finally:
        assert sys.excepthook in (on_hook, sys.__excepthook__)
        sys.excepthook = sys.__excepthook__
        if exceptions:
            reraise(*exceptions[0])
