import unittest

import gi.docstring
from gi.docstring import _get_pytype_hint
from gi.repository import GIMarshallingTests
from gi.repository import Gio


class Test(unittest.TestCase):
    def test_api(self):
        new_func = lambda info: 'docstring test'
        old_func = gi.docstring.get_doc_string_generator()

        gi.docstring.set_doc_string_generator(new_func)
        self.assertEqual(gi.docstring.get_doc_string_generator(),
                         new_func)
        self.assertEqual(gi.docstring.generate_doc_string(None),
                         'docstring test')

        # Set back to original generator
        gi.docstring.set_doc_string_generator(old_func)
        self.assertEqual(gi.docstring.get_doc_string_generator(),
                         old_func)

    def test_split_args_multi_out(self):
        in_args, out_args = gi.docstring.split_function_info_args(GIMarshallingTests.int_out_out)
        self.assertEqual(len(in_args), 0)
        self.assertEqual(len(out_args), 2)
        self.assertEqual(_get_pytype_hint(out_args[0].get_type()), 'int')
        self.assertEqual(_get_pytype_hint(out_args[1].get_type()), 'int')

    def test_split_args_inout(self):
        in_args, out_args = gi.docstring.split_function_info_args(GIMarshallingTests.long_inout_max_min)
        self.assertEqual(len(in_args), 1)
        self.assertEqual(len(out_args), 1)
        self.assertEqual(in_args[0].get_name(), out_args[0].get_name())
        self.assertEqual(_get_pytype_hint(in_args[0].get_type()),
                         _get_pytype_hint(out_args[0].get_type()))

    def test_split_args_none(self):
        obj = GIMarshallingTests.Object(int=33)
        in_args, out_args = gi.docstring.split_function_info_args(obj.none_inout)
        self.assertEqual(len(in_args), 1)
        self.assertEqual(len(out_args), 1)

    def test_final_signature_with_full_inout(self):
        self.assertEqual(GIMarshallingTests.Object.full_inout.__doc__,
                         'full_inout(object:GIMarshallingTests.Object) -> object:GIMarshallingTests.Object')

    def test_overridden_doc_is_not_clobbered(self):
        self.assertEqual(GIMarshallingTests.OverridesObject.method.__doc__,
                         'Overridden doc string.')

    def test_allow_none_with_user_data_defaults(self):
        g_file_copy_doc = 'copy(self, destination:Gio.File, ' \
                          'flags:Gio.FileCopyFlags, ' \
                          'cancellable:Gio.Cancellable=None, ' \
                          'progress_callback:Gio.FileProgressCallback=None, ' \
                          'progress_callback_data=None)'

        self.assertEqual(Gio.File.copy.__doc__, g_file_copy_doc)

    def test_array_length_arg(self):
        self.assertEqual(GIMarshallingTests.array_in.__doc__,
                         'array_in(ints:list)')

    def test_init_function(self):
        # This tests implicit array length args along with skipping a
        # boolean return
        self.assertEqual(GIMarshallingTests.init_function.__doc__,
                         'init_function(argv:list=None) -> argv:list')
