
import pytest
from gi.repository import Regress


def test_fundamental_type_instantiation_fails():
    with pytest.raises(TypeError, match="No means to translate argument or return value for 'RegressTestFundamentalSubObject'"):
        Regress.TestFundamentalSubObject.new("data")
