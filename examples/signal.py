import gobject

class C(gobject.GObject):
    def __init__(self):
        self.__gobject_init__() # default constructor using our new GType
    def do_my_signal(self, arg):
	print "C: class closure for `my_signal' called with argument", arg

gobject.type_register(C)
gobject.signal_new("my_signal", C, gobject.SIGNAL_RUN_FIRST,
		   gobject.TYPE_NONE, (gobject.TYPE_INT, ))

class D(C):
    def do_my_signal(self, arg):
	print "D: class closure for `my_signal' called.  Chaining up to C"
	C.do_my_signal(self, arg)

gobject.type_register(D)

def my_signal_handler(object, arg, *extra):
    print "handler for `my_signal' called with argument", arg, \
	  "and extra args", extra

inst = C()
inst2 = D()

inst.connect("my_signal", my_signal_handler, 1, 2, 3)
inst.emit("my_signal", 42)
inst2.emit("my_signal", 42)
