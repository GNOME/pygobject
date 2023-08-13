from gi.repository import Regress


def test_refcount_no_data():
    obj = Regress.TestFundamentalSubObject()

    assert isinstance(obj, Regress.TestFundamentalSubObject)
    assert isinstance(obj, Regress.TestFundamentalObject)
    assert obj.refcount == 1
    assert obj.data is None


def test_refcount_with_data():
    obj = Regress.TestFundamentalSubObject.new('foo')
    assert isinstance(obj, Regress.TestFundamentalSubObject)
    assert isinstance(obj, Regress.TestFundamentalObject)
    assert obj.refcount == 1
    assert obj.data == 'foo'


# Add more test cases:
#  - setting properties (obj.data = ...), to test the change in pygi_set_property_value_real(), using compatible and incompatible types
#  - getting values of different data types, including None and objects
#  - Calling methods on the Fundamental, testing none and full transfer modes for an argument and return value
#  - constructor and method calls with keyword arguments
#  - Passing a Fundamental object in an array to a method; array handling keeps causing crashes and malfunctions, so I think it's important to make sure that this keeps working.
