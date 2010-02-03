from ..types import override
from ..importer import modules

TestGI = modules['TestGI']


OVERRIDES_CONSTANT = 7


class OverridesStruct(TestGI.OverridesStruct):

    def __new__(cls, long_):
        return TestGI.OverridesStruct.__new__(cls)

    def __init__(self, long_):
        TestGI.OverridesStruct.__init__(self)
        self.long_ = long_

    def method(self):
        return TestGI.OverridesStruct.method(self) / 7

OverridesStruct = override(OverridesStruct)


class OverridesObject(TestGI.OverridesObject):

    def __new__(cls, long_):
        return TestGI.OverridesObject.__new__(cls)

    def __init__(self, long_):
        TestGI.OverridesObject.__init__(self)
        # FIXME: doesn't work yet
        #self.long_ = long_

    @classmethod
    def new(cls, long_):
        self = TestGI.OverridesObject.new()
        # FIXME: doesn't work yet
        #self.long_ = long_
        return self

    def method(self):
        return TestGI.OverridesObject.method(self) / 7

OverridesObject = override(OverridesObject)


__all__ = [OVERRIDES_CONSTANT, OverridesStruct, OverridesObject]

