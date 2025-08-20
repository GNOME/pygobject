import inspect
import typing
import unittest

from gi.repository import GIMarshallingTests, GLib, Gio, GObject


class Test(unittest.TestCase):
    def test_signature_attr(self):
        func = GIMarshallingTests.Object.full_inout
        self.assertTrue(hasattr(func, "__signature__"))
        sig1 = func.__signature__
        sig2 = inspect.signature(func)
        self.assertIsInstance(sig1, inspect.Signature)
        self.assertEqual(sig1, sig2)

    def assertSignatureEqual(self, first, second, msg=None):
        first_sig = inspect.signature(first)
        second_sig = inspect.signature(second)
        self.assertEqual(first_sig, second_sig, msg=msg)

    def test_allow_none_with_user_data_defaults(self):
        def expected(
            self,
            destination: Gio.File,
            flags: Gio.FileCopyFlags,
            cancellable: typing.Optional[Gio.Cancellable] = None,
            progress_callback: typing.Optional[
                typing.Callable[[int, int, typing.Any], None]
            ] = None,
            progress_callback_data: typing.Any = None,
        ) -> bool:
            pass

        self.assertSignatureEqual(Gio.File.copy, expected)

    def test_init_function(self):
        def expected(
            argv: typing.Optional[list[str]] = None,
        ) -> tuple[bool, typing.Optional[list[str]]]:
            pass

        self.assertSignatureEqual(GIMarshallingTests.init_function, expected)

    def test_in_arg(self):
        def expected(ints: list[int]) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.array_in, expected)

    def test_inout_arg(self):
        def expected(ints: list[int]) -> list[int]:
            pass

        self.assertSignatureEqual(GIMarshallingTests.array_inout, expected)

    def test_out_arg(self):
        def expected() -> list[int]:
            pass

        self.assertSignatureEqual(GIMarshallingTests.array_out, expected)

    def test_arg_boolean(self):
        def expected(v: bool) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.boolean_in_true, expected)

    def test_arg_int8(self):
        def expected(v: int) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.int8_in_max, expected)

    def test_arg_uint8(self):
        def expected(v: int) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.uint8_in, expected)

    def test_arg_int16(self):
        def expected(v: int) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.int16_in_max, expected)

    def test_arg_uint16(self):
        def expected(v: int) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.uint16_in, expected)

    def test_arg_int32(self):
        def expected(v: int) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.int32_in_max, expected)

    def test_arg_uint32(self):
        def expected(v: int) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.uint32_in, expected)

    def test_arg_int64(self):
        def expected(v: int) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.int64_in_max, expected)

    def test_arg_uint64(self):
        def expected(v: int) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.uint64_in, expected)

    def test_arg_float(self):
        def expected(v: float) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.float_in, expected)

    def test_arg_double(self):
        def expected(v: float) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.double_in, expected)

    def test_arg_gtype(self):
        def expected(gtype: GObject.GType) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.gtype_in, expected)

    def test_arg_utf8(self):
        def expected(utf8: str) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.utf8_none_in, expected)

    def test_arg_filename(self):
        def expected(path: str) -> bool:
            pass

        self.assertSignatureEqual(GIMarshallingTests.filename_exists, expected)

    def test_arg_enum(self):
        def expected(v: GIMarshallingTests.Enum) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.enum_in, expected)

    def test_arg_genum(self):
        def expected(v: GIMarshallingTests.GEnum) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.genum_in, expected)

    def test_arg_flags(self):
        def expected(v: GIMarshallingTests.NoTypeFlags) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.no_type_flags_in, expected)

    def test_arg_gflags(self):
        def expected(v: GIMarshallingTests.Flags) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.flags_in, expected)

    def test_arg_array(self):
        def expected(ints: list[int]) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.array_fixed_int_in, expected)
        self.assertSignatureEqual(GIMarshallingTests.array_in, expected)

    def test_arg_garray(self):
        def expected(array_: list[int]) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.garray_int_none_in, expected)

    def test_arg_ptrarray(self):
        def expected(parray_: list[str]) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.gptrarray_utf8_none_in, expected)

    def test_arg_glist(self):
        def expected(list: list[int]) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.glist_int_none_in, expected)

    def test_arg_gslist(self):
        def expected(list: list[int]) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.gslist_int_none_in, expected)

    def test_arg_ghashtable(self):
        def expected(hash_table: dict[int, int]) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.ghashtable_int_none_in, expected)

    def test_arg_gvalue(self):
        def expected(value: GObject.Value) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.gvalue_in, expected)

    def test_arg_struct(self):
        def expected(structs: list[GIMarshallingTests.SimpleStruct]) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.array_simple_struct_in, expected)

    def test_arg_boxed(self):
        def expected(structs: list[GIMarshallingTests.BoxedStruct]) -> None:
            pass

        self.assertSignatureEqual(GIMarshallingTests.array_struct_in, expected)

    def test_return(self):
        def expected() -> int:
            pass

        self.assertSignatureEqual(GIMarshallingTests.int_return_max, expected)

    def test_return_multiple(self):
        def expected() -> tuple[int, int]:
            pass

        self.assertSignatureEqual(GIMarshallingTests.int_out_out, expected)
        self.assertSignatureEqual(GIMarshallingTests.int_return_out, expected)

    def test_object_full_inout(self):
        class A:
            @staticmethod
            def expected(
                object: GIMarshallingTests.Object,
            ) -> GIMarshallingTests.Object:
                pass

        self.assertSignatureEqual(GIMarshallingTests.Object.full_inout, A.expected)
        self.assertSignatureEqual(GIMarshallingTests.Object().full_inout, A().expected)

    def test_object_constructor(self):
        class A:
            @classmethod
            def expected(cls, int_: int) -> GIMarshallingTests.Object:
                pass

        self.assertSignatureEqual(GIMarshallingTests.Object.new, A.expected)
        self.assertSignatureEqual(GIMarshallingTests.Object().new, A().expected)

    def test_object_method(self):
        class A:
            def expected(self, ints: list[int]) -> None:
                pass

        self.assertSignatureEqual(GIMarshallingTests.Object.method_array_in, A.expected)
        self.assertSignatureEqual(
            GIMarshallingTests.Object().method_array_in, A().expected
        )

    def test_object_static_method(self):
        class A:
            @staticmethod
            def expected() -> None:
                pass

        self.assertSignatureEqual(GIMarshallingTests.Object.static_method, A.expected)
        self.assertSignatureEqual(
            GIMarshallingTests.Object().static_method, A().expected
        )

    def test_object_virtual_method(self):
        class A:
            @classmethod
            def expected(cls, self, object: GObject.GObject) -> None:
                pass

        self.assertSignatureEqual(
            GIMarshallingTests.Object.do_vfunc_in_object_transfer_none, A.expected
        )
        self.assertSignatureEqual(
            GIMarshallingTests.Object().do_vfunc_in_object_transfer_none, A().expected
        )

    def test_arg_conflict(self):
        class A:
            @classmethod
            def expected(
                type_,
                self,
                offset: int,
                type: GLib.SeekType,
                cancellable: typing.Optional[Gio.Cancellable] = None,
            ) -> bool:
                pass

        self.assertSignatureEqual(Gio.FileIOStream.do_seek, A.expected)
