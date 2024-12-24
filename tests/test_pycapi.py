import unittest
import ctypes
from ctypes import c_void_p, py_object, c_char_p

import gi
from gi.repository import Gio


def get_capi():
    if not hasattr(ctypes, "pythonapi"):
        return None

    class CAPI(ctypes.Structure):
        _fields_ = [
            ("", c_void_p),
            ("", c_void_p),
            ("", c_void_p),
            ("newgobj", ctypes.PYFUNCTYPE(py_object, c_void_p)),
        ]

    api_obj = gi._gobject._PyGObject_API
    func_type = ctypes.PYFUNCTYPE(c_void_p, py_object, c_char_p)
    PyCapsule_GetPointer = func_type(("PyCapsule_GetPointer", ctypes.pythonapi))
    ptr = PyCapsule_GetPointer(api_obj, b"gobject._PyGObject_API")

    ptr = ctypes.cast(ptr, ctypes.POINTER(CAPI))
    return ptr.contents


API = get_capi()


@unittest.skipUnless(API, "no pythonapi support")
class TestPythonCAPI(unittest.TestCase):
    def test_newgobj(self):
        w = Gio.FileInfo()
        # XXX: ugh :/
        ptr = int(repr(w).split()[-1].split(")")[0], 16)

        capi = get_capi()
        new_w = capi.newgobj(ptr)
        assert w == new_w
