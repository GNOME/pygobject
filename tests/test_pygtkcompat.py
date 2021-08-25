# -*- Mode: Python; py-indent-offset: 4 -*-
# vim: tabstop=4 shiftwidth=4 expandtab

import pytest

import pygtkcompat


def test_pygtkcompat():
    with pytest.raises(RuntimeError, match="pygtkcompat is deprecated"):
        pygtkcompat.enable()
