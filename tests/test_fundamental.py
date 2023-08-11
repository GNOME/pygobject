from gi.repository import Regress as Everything


def test_refcount_no_data():
    obj = Everything.TestFundamentalSubObject()

    assert isinstance(obj, Everything.TestFundamentalSubObject)
    assert isinstance(obj, Everything.TestFundamentalObject)
    assert obj.refcount == 1
    assert obj.data is None

def test_refcount_with_data():
    obj = Everything.TestFundamentalSubObject.new('foo')
    assert isinstance(obj, Everything.TestFundamentalSubObject)
    assert isinstance(obj, Everything.TestFundamentalObject)
    assert obj.refcount == 1
    assert obj.data == 'foo'
