import pytest

import pygtkcompat


def test_pygtkcompat():
    with pytest.raises(RuntimeError, match="pygtkcompat is deprecated"):
        pygtkcompat.enable()
