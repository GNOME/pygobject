from gi.repository import GLib, Gio, Regress


def iteration():
    ctx = GLib.main_context_default()
    while ctx.pending():
        ctx.iteration(False)


def test_async_callback():
    result = []
    cancel = Gio.Cancellable()

    def callback(obj, res):
        result.append(obj.function_finish(res))

    obj = Regress.TestObj()
    obj.function_async(GLib.PRIORITY_DEFAULT, cancellable=cancel, callback=callback)
    obj.function_thaw_async()

    iteration()

    assert result == [True]


def test_async_callback_with_extra_callbacks():
    result = []
    cancel = Gio.Cancellable()

    def test_cb():
        pass

    def callback(obj, res):
        result.append(obj.function2_finish(res))

    obj = Regress.TestObj()
    obj.function2(GLib.PRIORITY_DEFAULT, cancellable=cancel, test_cb=test_cb, callback=callback)
    obj.function2_thaw_async()

    iteration()

    assert result == [(True, None)]
