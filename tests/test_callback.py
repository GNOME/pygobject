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

    def callback(obj, res):
        result.append(obj.function2_finish(res))

    obj = Regress.TestObj()
    obj.function2(GLib.PRIORITY_DEFAULT, None, None, None, callback)
    obj.function_thaw_async()

    iteration()

    assert result == [(True, None)]


def test_async_callback_with_extra_callbacks_filled_in():
    result = []
    cancel = Gio.Cancellable()

    def test_cb(data):
        result.append(data)
        return 0

    def callback(obj, res):
        result.append(obj.function2_finish(res))

    obj = Regress.TestObj()
    obj.function2(GLib.PRIORITY_DEFAULT, cancel, test_cb, ("test_cb data",), callback)
    obj.function_thaw_async()

    iteration()

    assert result == ["test_cb data", (True, None)]


def test_async_callback_with_extra_callbacks_as_kwarg():
    result = []
    cancel = Gio.Cancellable()

    def test_cb(data):
        result.append(data)
        return 0

    def callback(obj, res):
        result.append(obj.function2_finish(res))

    obj = Regress.TestObj()
    obj.function2(
        GLib.PRIORITY_DEFAULT, cancellable=cancel, test_cb=test_cb, callback=callback
    )
    obj.function_thaw_async()

    iteration()

    assert result == [None, (True, None)]


def test_async_callback_with_extra_callbacks_as_kwarg_and_user_data():
    result = []
    cancel = Gio.Cancellable()

    def test_cb(data):
        result.append(data)
        return 0

    def callback(obj, res, data):
        result.append(obj.function2_finish(res))

    obj = Regress.TestObj()
    obj.function2(
        GLib.PRIORITY_DEFAULT,
        cancellable=cancel,
        test_cb=test_cb,
        callback=callback,
        user_data=None,
    )
    obj.function_thaw_async()

    iteration()

    assert result == [None, (True, None)]
