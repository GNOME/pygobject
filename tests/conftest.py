# -*- coding: utf-8 -*-

from __future__ import absolute_import

import sys

import pytest

from gi._compat import reraise


@pytest.hookimpl(hookwrapper=True, tryfirst=True)
def pytest_runtest_call(item):
    """A pytest hook which takes over sys.excepthook and raises any uncaught
    exception (with PyGObject this happesn often when we get called from C,
    like any signal handler, vfuncs tc)
    """

    exceptions = []

    def on_hook(type_, value, tback):
        exceptions.append((type_, value, tback))

    orig_excepthook = sys.excepthook
    sys.excepthook = on_hook
    try:
        yield
    finally:
        sys.excepthook = orig_excepthook
        if exceptions:
            reraise(*exceptions[0])
