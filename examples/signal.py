from __future__ import print_function

from gi.repository import GObject


class C(GObject.GObject):
    @GObject.Signal(arg_types=(int,))
    def my_signal(self, arg):
        """Decorator style signal which uses the method name as signal name and
        the method as the closure.

        Note that with python3 annotations can be used for argument types as follows:
            @GObject.Signal
            def my_signal(self, arg:int):
                pass
        """
        print("C: class closure for `my_signal' called with argument", arg)

    @GObject.Signal
    def noarg_signal(self):
        """Decoration of a signal using all defaults and no arguments."""
        print("C: class closure for `noarg_signal' called")


class D(C):
    def do_my_signal(self, arg):
        print("D: class closure for `my_signal' called.  Chaining up to C")
        C.my_signal(self, arg)


def my_signal_handler(obj, arg, *extra):
    print("handler for `my_signal' called with argument", arg, "and extra args", extra)


inst = C()
inst2 = D()

inst.connect("my_signal", my_signal_handler, 1, 2, 3)
inst.connect("noarg_signal", my_signal_handler, 1, 2, 3)
inst.emit("my_signal", 42)
inst.emit("noarg_signal")
inst2.emit("my_signal", 42)
