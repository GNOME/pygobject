import unittest

import gi.docstring

from gi.repository import Regress
from gi.repository import GIMarshallingTests
from gi.repository import Gio
from gi.repository import GObject

try:
    from gi.repository import Gtk
except ImportError:
    Gtk = None


class Test(unittest.TestCase):
    def test_api(self):
        def new_func(info):
            return "docstring test"

        old_func = gi.docstring.get_doc_string_generator()

        gi.docstring.set_doc_string_generator(new_func)
        self.assertEqual(gi.docstring.get_doc_string_generator(), new_func)
        self.assertEqual(gi.docstring.generate_doc_string(None), "docstring test")

        # Set back to original generator
        gi.docstring.set_doc_string_generator(old_func)
        self.assertEqual(gi.docstring.get_doc_string_generator(), old_func)

    def test_final_signature_with_full_inout(self):
        self.assertEqual(
            GIMarshallingTests.Object.full_inout.__doc__,
            "full_inout(object:GIMarshallingTests.Object) -> object:GIMarshallingTests.Object",
        )

    def test_overridden_doc_is_not_clobbered(self):
        self.assertEqual(
            GIMarshallingTests.OverridesObject.method.__doc__, "Overridden doc string."
        )

    def test_allow_none_with_user_data_defaults(self):
        g_file_copy_doc = (
            "copy(self, destination:Gio.File, "
            "flags:Gio.FileCopyFlags, "
            "cancellable:Gio.Cancellable=None, "
            "progress_callback:Gio.FileProgressCallback=None, "
            "progress_callback_data=None) -> bool"
        )

        self.assertEqual(Gio.File.copy.__doc__, g_file_copy_doc)

    def test_array_length_arg(self):
        self.assertEqual(GIMarshallingTests.array_in.__doc__, "array_in(ints:list)")

    def test_init_function(self):
        # This tests implicit array length args along with skipping a
        # boolean return
        self.assertEqual(
            GIMarshallingTests.init_function.__doc__,
            "init_function(argv:list=None) -> bool, argv:list",
        )

    def test_boolean_return(self):
        self.assertEqual(
            GIMarshallingTests.boolean_return_true.__doc__,
            "boolean_return_true() -> bool",
        )

    # https://bugzilla.gnome.org/show_bug.cgi?id=740301
    def test_may_return_none(self):
        self.assertEqual(
            Gio.File.get_basename.__doc__, "get_basename(self) -> str or None"
        )

    def test_class_doc_constructors(self):
        doc = GIMarshallingTests.Object.__doc__
        self.assertTrue("new(int_:int)" in doc)

    def test_struct_doc_constructors(self):
        doc = GIMarshallingTests.BoxedStruct.__doc__
        self.assertTrue("new()" in doc)
        self.assertTrue("BoxedStruct()" in doc)

    def test_private_struct_constructors(self):
        # Structs without a size or constructor should have no constructor docs.
        doc = Regress.TestBoxedPrivate.__doc__
        self.assertEqual(doc, "")

    def test_array_inout_etc(self):
        self.assertEqual(
            GIMarshallingTests.array_inout_etc.__doc__,
            "array_inout_etc(first:int, ints:list, last:int) -> ints:list, sum:int",
        )

    def test_array_out_etc(self):
        self.assertEqual(
            GIMarshallingTests.array_out_etc.__doc__,
            "array_out_etc(first:int, last:int) -> ints:list, sum:int",
        )

    @unittest.skipUnless(Gtk, "no Gtk")
    def test_shared_array_length_with_prior_out_arg(self):
        # Test the 'iter' out argument does not effect length argument skipping.
        self.assertRegex(
            Gtk.ListStore.insert_with_valuesv.__doc__,
            "insert_with_values.*\\(self, position:int, columns:list, values:list\\) -> iter:Gtk.TreeIter",
        )

    def test_sub_class_doc(self):
        class A(GObject.Object):
            """first doc."""

            pass

        class B(A):
            """second doc."""

            pass

        self.assertEqual(A.__doc__, "first doc.")
        self.assertEqual(B.__doc__, "second doc.")

    def test_sub_class_no_doc(self):
        class A(GObject.Object):
            pass

        class B(A):
            """sub-class doc."""

        self.assertEqual(A.__doc__, None)
        self.assertEqual(B.__doc__, "sub-class doc.")

    @unittest.expectedFailure  # https://bugzilla.gnome.org/show_bug.cgi?id=734926
    def test_sub_class_doc_setattr(self):
        class A(GObject.Object):
            pass

        class B(A):
            pass

        A.__doc__ = "custom doc"

        self.assertEqual(A.__doc__, "custom doc")
        self.assertEqual(B.__doc__, "custom doc")

    def test_return_array_with_length_argument(self):
        self.assertEqual(
            GIMarshallingTests.enum_array_return_type.__doc__,
            "enum_array_return_type() -> list",
        )
